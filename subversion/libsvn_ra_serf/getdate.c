/*
 * getdate.c :  entry point for get_dated_revision for ra_serf
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */



#include <apr_uri.h>

#include <expat.h>

#include <serf.h>

#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_time.h"
#include "svn_xml.h"

#include "private/svn_dav_protocol.h"

#include "svn_private_config.h"

#include "../libsvn_ra/ra_loader.h"

#include "ra_serf.h"


/*
 * This enum represents the current state of our XML parsing for a REPORT.
 */
typedef enum date_state_e {
  NONE = 0,
  VERSION_NAME
} date_state_e;


typedef struct date_context_t {
  /* The time asked about. */
  apr_time_t time;

  /* What was the youngest revision at that time? */
  svn_revnum_t *revision;

  /* are we done? */
  svn_boolean_t done;

} date_context_t;


static svn_error_t *
start_getdate(svn_ra_serf__xml_parser_t *parser,
              svn_ra_serf__dav_props_t name,
              const char **attrs,
              apr_pool_t *scratch_pool)
{
  date_context_t *date_ctx = parser->user_data;
  date_state_e state = parser->state->current_state;

  UNUSED_CTX(date_ctx);

  if (state == NONE &&
      strcmp(name.name, SVN_DAV__VERSION_NAME) == 0)
    {
      svn_ra_serf__xml_push_state(parser, VERSION_NAME);

      parser->state->private = svn_stringbuf_create_empty(parser->state->pool);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_getdate(svn_ra_serf__xml_parser_t *parser,
            svn_ra_serf__dav_props_t name,
            apr_pool_t *scratch_pool)
{
  date_context_t *date_ctx = parser->user_data;
  date_state_e state = parser->state->current_state;

  if (state == VERSION_NAME &&
      strcmp(name.name, SVN_DAV__VERSION_NAME) == 0)
    {
      const svn_stringbuf_t *datebuf = parser->state->private;

      *date_ctx->revision = SVN_STR_TO_REV(datebuf->data);
      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_getdate(svn_ra_serf__xml_parser_t *parser,
              const char *data,
              apr_size_t len,
              apr_pool_t *scratch_pool)
{
  date_context_t *date_ctx = parser->user_data;
  date_state_e state = parser->state->current_state;
  svn_stringbuf_t *datebuf;

  UNUSED_CTX(date_ctx);

  switch (state)
    {
    case VERSION_NAME:
        datebuf = parser->state->private;
        svn_stringbuf_appendbytes(datebuf, data, len);
        break;
    default:
        break;
    }

  return SVN_NO_ERROR;
}

/* Implements svn_ra_serf__request_body_delegate_t */
static svn_error_t *
create_getdate_body(serf_bucket_t **body_bkt,
                    void *baton,
                    serf_bucket_alloc_t *alloc,
                    apr_pool_t *pool)
{
  serf_bucket_t *buckets;
  date_context_t *date_ctx = baton;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(buckets, alloc, "S:dated-rev-report",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    "xmlns:D", "DAV:",
                                    NULL);

  svn_ra_serf__add_tag_buckets(buckets,
                               "D:" SVN_DAV__CREATIONDATE,
                               svn_time_to_cstring(date_ctx->time, pool),
                               alloc);

  svn_ra_serf__add_close_tag_buckets(buckets, alloc, "S:dated-rev-report");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_dated_revision(svn_ra_session_t *ra_session,
                                svn_revnum_t *revision,
                                apr_time_t tm,
                                apr_pool_t *pool)
{
  date_context_t *date_ctx;
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  const char *report_target;

  date_ctx = apr_palloc(pool, sizeof(*date_ctx));
  date_ctx->time = tm;
  date_ctx->revision = revision;
  date_ctx->done = FALSE;

  SVN_ERR(svn_ra_serf__report_resource(&report_target, session, NULL, pool));

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->handler_pool = pool;
  handler->method = "REPORT";
  handler->path = report_target;
  handler->body_type = "text/xml";
  handler->conn = session->conns[0];
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = date_ctx;
  parser_ctx->start = start_getdate;
  parser_ctx->end = end_getdate;
  parser_ctx->cdata = cdata_getdate;
  parser_ctx->done = &date_ctx->done;

  handler->body_delegate = create_getdate_body;
  handler->body_delegate_baton = date_ctx;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  *date_ctx->revision = SVN_INVALID_REVNUM;

  /* ### use svn_ra_serf__error_on_status() ?  */

  return svn_ra_serf__context_run_wait(&date_ctx->done, session, pool);
}
