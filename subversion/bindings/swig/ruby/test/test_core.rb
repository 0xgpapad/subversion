require "my-assertions"
require "util"

require "svn/core"

class SvnCoreTest < Test::Unit::TestCase
  include SvnTestUtil
  
  def setup
    @repos_path = File.join("test", "repos")
    setup_repository(@repos_path)
  end

  def teardown
    GC.enable
    teardown_repository(@repos_path)
  end
  
  def test_binary_mime_type?
    assert(Svn::Core.binary_mime_type?("image/png"))
    assert(!Svn::Core.binary_mime_type?("text/plain"))
  end

  def test_not_new_auth_provider_object
    assert_raise(NoMethodError) do
      Svn::Core::AuthProviderObject.new
    end
  end

  def test_pool_GC
    GC.disable

    made_number_of_pool = 200
    pools = []
    
    gc
    before_number_of_pools = number_of_pools
    (made_number_of_pool / 2).times do
      pools << parent_used_child_pool
    end
    STDERR.puts [pools.size, number_of_pools - before_number_of_pools].inspect
    gc
    current_number_of_pools = number_of_pools
    created_number_of_pools = current_number_of_pools - before_number_of_pools
    STDERR.puts [pools.size, created_number_of_pools].inspect
    assert_equal(made_number_of_pool, created_number_of_pools)

    gc
    pools.clear
    before_number_of_pools = number_of_pools
    gc
    current_number_of_pools = number_of_pools
    recycled_number_of_pools = before_number_of_pools - current_number_of_pools
    assert_operator(made_number_of_pool * 0.8, :<=, recycled_number_of_pools)
  end

  def parent_used_child_pool
    parent_pool = Svn::Core::Pool.new
    child_pool = Svn::Core::Pool.new(parent_pool)
    now = Time.now.gmtime
    apr_time = now.to_i * Svn::Util::MILLION + now.usec
    Svn::Core.time_to_human_cstring(apr_time, parent_pool)
    child_pool
  end

  def number_of_pools
    ObjectSpace.each_object(Svn::Core::Pool) {}
  end

  def gc
    before_disabled = GC.enable
    GC.start
    GC.disable if before_disabled
  end
end
