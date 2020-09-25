/// @file
/// @brief Allows test scripts to define an Azalea system suitable for their test
///
/// Test systems can be constructed with a mixture of live and dummy components.

#include <type_traits>

#include "work_queue.h"

template<typename wq> class test_system_factory
{
public:
  test_system_factory()
  {
    create_work_queue();
  };

  virtual ~test_system_factory()
  {
    if (created_work_queue)
    {
      work::test_only_terminate_queue();
    }
  };

protected:
  bool created_work_queue{false};

  template<class Q = wq>
    typename std::enable_if<std::is_same<Q, void>::value, void>::type create_work_queue() { };

  template<class Q = wq>
    typename std::enable_if<!std::is_same<Q, void>::value, void>::type create_work_queue()
  {
    work::init_queue<wq>();
    created_work_queue = true;
  };

};
