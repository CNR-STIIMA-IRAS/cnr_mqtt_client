/*
 *  Software License Agreement (New BSD License)
 *
 *  Copyright 2020 National Council of Research of Italy (CNR)
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder(s) nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <iostream>
#include <time.h>
#include <numeric>
#include <gtest/gtest.h>

std::string path_to_src = "../test/config";

namespace detail
{
  struct unwrapper
  {
    explicit unwrapper(std::exception_ptr pe) : pe_(pe) {}

    operator bool() const
    {
      return bool(pe_);
    }

    friend auto operator<<(std::ostream& os, unwrapper const& u) -> std::ostream&
    {
      try
      {
          std::rethrow_exception(u.pe_);
          return os << "no exception";
      }
      catch(std::runtime_error const& e)
      {
          return os << "runtime_error: " << e.what();
      }
      catch(std::logic_error const& e)
      {
          return os << "logic_error: " << e.what();
      }
      catch(std::exception const& e)
      {
          return os << "exception: " << e.what();
      }
      catch(...)
      {
          return os << "non-standard exception";
      }
    }
    std::exception_ptr pe_;
  };
}

auto unwrap(std::exception_ptr pe)
{
  return detail::unwrapper(pe);
}

template<class F>
::testing::AssertionResult does_not_throw(F&& f)
{
  try
  {
     f();
     return ::testing::AssertionSuccess();
  }
  catch(...)
  {
     return ::testing::AssertionFailure() << unwrap(std::current_exception());
  }
}

std::map<std::string, std::map<std::string, std::vector<double> > > statistics;

#define EXECUTION_TIME( id1, id2, ... )\
{\
  struct timespec start, end;\
  clock_gettime(CLOCK_MONOTONIC, &start);\
  __VA_ARGS__;\
  clock_gettime(CLOCK_MONOTONIC, &end);\
  double time_taken;\
  time_taken = double(end.tv_sec - start.tv_sec) * 1e9;\
  time_taken = double(time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;\
  statistics[id1][id2].push_back(time_taken * 1.0e3);\
}

void printStatistics()
{
  for(const auto & p : statistics)
  {
    std::cout<< std::endl
            << "-------------------------------------" << std::endl
            <<"Flush Type: " << p.first << " " << std::endl
            << "-------------------------------------" << std::endl;
    for(const auto & k : p.second)
    {
      if(k.second.size() > 0 )
      {
        auto max = std::max_element(std::begin(k.second), std::end(k.second));
        auto min = std::min_element(std::begin(k.second), std::end(k.second));
        double mean = std::accumulate(std::begin(k.second), std::end(k.second), 0.0);
        mean = mean / double(k.second.size());
        int sz = std::printf("%36s [%5zu]: %3.3fms, %3.3fms, %3.3fms\n", k.first.c_str(), k.second.size(), *min, mean, * max);
        if(sz<=0)
        {
          std::cerr << "Error in printing the report .." << std::endl;
        }
      }
    }
  }
}

#include <cnr_mqtt_client/cnr_mqtt_client.h>

std::shared_ptr<cnr::mqtt::MQTTClient> _mqtt;

#if 0
// Declare another test
TEST(TestSuite, creator)
{ 
  _mqtt = new cnr::drapebot::MQTTDrapebotClientHw(cid.c_str(), host_str.c_str(), port, 60, use_json_);
  EXPECT_TRUE(does_not_throw([&]{ _mqtt.reset(new cnr::mqtt::MQTTClient() ); }));
}
  
TEST(TestSuite, subscribe)
{
  EXPECT_TRUE(_mqtt->subscribe(NULL, m_mqtt_feedback_topic.c_str(), 1) != 0);
}

TEST(TestSuite, waitForFirstMsgRec)
{
  EXPECT_TRUE(does_not_throw([&]
  { 
    do 
    {
      if (_mqtt->loop(1) != MOSQ_ERR_SUCCESS)
      {       
        std::this_thread::sleep_for(std::chrono::milliseconds(5))
      }
      
      cnr::drapebot::drapebot_msg_hw last_msg;
      if (_mqtt->isFirstMsgRec())
      {
        _mqtt->getLastReceivedMessage(last_msg);
      }
    } while ( true );
      return true;
  }));
}

TEST(TestSuite, loop)
{

    if (_mqtt->loop(1) != MOSQ_ERR_SUCCESS)
      CNR_WARN(m_logger,"_mqtt->loop() failed. check it");
  
    cnr::drapebot::drapebot_msg_hw last_msg;

}

TEST(TestSuite, destructor)
{ 
  EXPECT_NO_FATAL_FAILURE(_mqtt.reset());
}
#endif

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  bool ok = false;
  for(size_t i=0u;i<100u;i++)
  {
    ok = RUN_ALL_TESTS();
  }
  printStatistics();

  return ok;
}

