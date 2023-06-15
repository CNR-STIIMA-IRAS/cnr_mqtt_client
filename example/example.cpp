#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>


#include "json.hpp"
#include <cnr_mqtt_client/cnr_mqtt_client.h>

using namespace std::chrono_literals;

union my_msg
{
  uint8_t data[sizeof(double) * 6 + sizeof(double) + sizeof(unsigned long int)];
  struct helper
  {
    double J1;
    double J2;
    double J3;
    double J4;
    double J5;
    double J6;
    double E0;
    unsigned long int count;
  } msg;
};

void vec_to_msg(const std::vector<double>& v, my_msg* msg)
{
  std::memcpy(msg->data, v.data(), 7 * sizeof(double));
}

auto now()
{
  return std::chrono::steady_clock::now();
}

auto awake_time()
{
  using std::chrono::operator""ms;
  return now() + 4ms;
}

class MyMsgDecoder : public cnr::mqtt::MsgDecoder
{
public:
  MyMsgDecoder(my_msg* msg, bool use_json) : msg_(msg), use_json_(use_json), first_msg_rec_(false){};
  virtual ~MyMsgDecoder()
  {
    msg_ = nullptr;
  }

  // The method should be reimplemented on the base of the application
  void on_message(const struct mosquitto_message* msg) override
  {
    if (use_json_)
    {
      char* buf = new char[msg->payloadlen];
      memcpy(&buf[0], msg->payload, msg->payloadlen);

      std::string buf_str(buf);

      if (mtx_.try_lock_for(std::chrono::milliseconds(2)))
      {
        nlohmann::json data = nlohmann::json::parse(buf_str);

        msg_->msg.J1 = data["J0"];
        msg_->msg.J2 = data["J1"];
        msg_->msg.J3 = data["J2"];
        msg_->msg.J4 = data["J3"];
        msg_->msg.J5 = data["J4"];
        msg_->msg.J6 = data["J5"];
        msg_->msg.E0 = data["E0"];
        msg_->msg.count = data["count"];

        if (!first_msg_rec_)
          first_msg_rec_ = true;

        mtx_.unlock();
      }
      else
        std::cerr << "Can't lock mutex in DrapebotMsgDecoderHw::on_message." << std::endl;
    }
    else
    {
      int n_joints = 7;
      int message_size = n_joints * sizeof(msg_->msg.E0) + sizeof(msg_->msg.count);

      std::vector<double> joints;

      if (msg->payloadlen == message_size)
      {
        if (mtx_.try_lock_for(std::chrono::milliseconds(2)))
        {
          memcpy(msg_->data, (char*)msg->payload, sizeof(my_msg));

          if (!first_msg_rec_)
            first_msg_rec_ = true;

          mtx_.unlock();
        }
      }
      else
      {
        std::cerr << "The message received from MQTT has a wrong length" << std::endl;
        setNewMessageAvailable(false);
        setDataValid(false);
      }
    }
  }
  bool isFirstMsgRec()
  {
    return first_msg_rec_;
  };

private:
  my_msg* msg_;
  bool use_json_;
  bool first_msg_rec_;
};

class MyMsgEncoder : public cnr::mqtt::MsgEncoder
{
public:
  MyMsgEncoder(my_msg* msg) : msg_(msg){};
  virtual ~MyMsgEncoder()
  {
    msg_ = nullptr;
  }

  // The method should be reimplemented on the base of the application
  void on_publish(int) override
  {
    // Nothing to do here
  }

private:
  my_msg* msg_;
};

class MyClient
{
public:
  MyClient(const char* id, const char* host, int port, int keepalive = 60, bool use_json = false)
  {
    try
    {
      msg_enc_ = new my_msg();
      msg_dec_ = new my_msg();

      my_msg_encoder_ = new MyMsgEncoder(msg_enc_);
      my_msg_decoder_ = new MyMsgDecoder(msg_dec_, use_json);

      mqtt_client_ = new cnr::mqtt::MQTTClient(id, host, port, my_msg_encoder_, my_msg_decoder_);

      msg_count_cmd = 0;
      first_message_received_ = false;
    }
    catch (const std::exception& e)
    {
      std::cerr << "Exception thrown in MyClient constructor: " << e.what() << std::endl;
    }
  }

  ~MyClient()
  {
    delete msg_dec_;
    delete msg_enc_;
    delete my_msg_decoder_;
    delete my_msg_encoder_;
    delete mqtt_client_;
  }

  int stop()
  {
    if (mqtt_client_ != NULL)
      return mqtt_client_->stop();
    else
      return -1;
  }

  int loop(int timeout = 4)
  {
    if (mqtt_client_ != NULL)
      return mqtt_client_->loop(timeout);
    else
      return -1;
  }

  // int reconnect(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff);
  int subscribe(int* mid, const char* sub, int qos)
  {
    if (mqtt_client_ != NULL)
      return mqtt_client_->subscribe(mid, sub, qos);
    else
      return -1;
  }

  int unsubscribe(int* mid, const char* sub)
  {
    if (mqtt_client_ != NULL)
      return mqtt_client_->unsubscribe(mid, sub);
    else
      return -1;
  }
  int publish(const void* payload, int payload_len, const char* topic_name)
  {
    if (mqtt_client_ != NULL)
      return mqtt_client_->publish(payload, payload_len, topic_name);
    else
      return -1;
  }
  void publish_with_tracking(const std::string& cmd_topic, my_msg& m)
  {
    msg_count_cmd += 1;
    m.msg.count = msg_count_cmd;

    int message_size_ = sizeof(m);

    void* payload_ = malloc(message_size_);
    memcpy(payload_, &m, message_size_);

    int n = cmd_topic.length();
    char* topic = new char[n + 1];
    strcpy(&topic[0], cmd_topic.c_str());

    int rc = publish(payload_, message_size_, topic);
    if (rc != 0)
      std::cerr << "MyClient::publish_with_tracking returned code:" << rc << std::endl;
  }

  bool getLastReceivedMessage(my_msg& feedback)
  {
    if (my_msg_decoder_ != NULL)
    {
      if (!my_msg_decoder_->isFirstMsgRec())
      {
        std::cerr << "First message not received yet." << std::endl;
        return false;
      }

      if (my_msg_decoder_->mtx_.try_lock_for(std::chrono::milliseconds(2)))
      {
        std::memcpy(&feedback.data, msg_dec_->data, sizeof(my_msg));

        my_msg_decoder_->mtx_.unlock();
        return true;
      }
      else
      {
        std::cerr << "Can't lock mutex in MyClient::getLastReceivedMessage. Last message received from MQTT not "
                     "recovered."
                  << std::endl;
        return false;
      }
    }
    std::cerr << "Decode null" << std::endl;
    return false;
  }
  bool isNewMessageAvailable()

  {
    if (my_msg_decoder_ != NULL)
      return my_msg_decoder_->isNewMessageAvailable();
    else
    {
      std::cerr << "my_msg_decoder_ == NULL . return false" << std::endl;
      return false;
    }
  }

  bool isDataValid()
  {
    if (my_msg_decoder_ != NULL)
      return my_msg_decoder_->isDataValid();
    else
      return false;
  }

  bool isFirstMsgRec()
  {
    return my_msg_decoder_->isFirstMsgRec();
  };

  int get_msg_count_cmd()
  {
    return msg_count_cmd;
  };
  void set_msg_count_cmd(const int& count)
  {
    msg_count_cmd = count;
  };

  bool getFirstMessageStatus()
  {
    return first_message_received_;
  }

  my_msg* msg_enc_;
  my_msg* msg_dec_;

private:
  std::mutex mtx_mqtt_;

  unsigned long int msg_count_cmd;

  bool first_message_received_;

  MyMsgDecoder* my_msg_decoder_;
  MyMsgEncoder* my_msg_encoder_;

  cnr::mqtt::MQTTClient* mqtt_client_;
};

constexpr int maximum_missing_cycle = 500;

int main()
{
  auto c_pid = fork();

  if (c_pid == -1)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if (c_pid > 0)
  {
    auto cl = new MyClient(nullptr, "localhost", 1883);
    std::cout << "***** Parent Process ****** CLIENT1 ****** PID: " << getpid() << std::endl;
    if (cl->subscribe(NULL, "/feedback", 1) != 0)
    {
      std::cerr << "CLIENT1 Error on Mosquitto subscribe topic: /feedback" << std::endl;
      return -1;
    }

    my_msg cmd_sent;
    my_msg feedback_recv;
    std::size_t cnt = 0;
    while (true)
    {
      cnt++;
      int delay = 0;
      const auto start{ now() };
      cl->publish_with_tracking("/command", cmd_sent);

      if (cl->loop(1) != MOSQ_ERR_SUCCESS)
      {
        std::cerr << "CLIENT1 loop() failed. check it" << std::endl;
      }

      if (cl->getLastReceivedMessage(feedback_recv))
      {
        if (!cl->isFirstMsgRec())
        {
          std::cout << "CLIENT1 First Message not yet recevied! " << std::endl;
        }
        else
        {
          if (cnt % 250 == 0)
          {
          }

          delay = std::fabs(cl->get_msg_count_cmd() - feedback_recv.msg.count);
          if (delay > maximum_missing_cycle)
          {
            std::cerr << "CLIENT1 delay: " << delay << " exceeds maximum missing cycle ( " << maximum_missing_cycle
                      << " ) . command: " << cl->get_msg_count_cmd() << ", feedback: " << feedback_recv.msg.count
                      << std::endl;
          }
        }
      }
      else
      {
        std::cerr << "CLIENT1 No new MQTT feedback message available OR first message not received yet... not good, "
                     "topic: /feedback";
      }
      cl->publish(feedback_recv.data, sizeof(my_msg), "/degub_feedback");

      std::this_thread::sleep_until(awake_time());
      std::chrono::duration<double, std::milli> elapsed{ now() - start };
      if (cnt % 250 == 0)
      {
        std::cout << "CLIENT1 SENT [ms]: " << cmd_sent.msg.count * 4
                  << " LAST RECEIVED  [ms]: " << feedback_recv.msg.count * 4 << "\tDelay [ms]: " << delay * 4
                  << std::endl;
      }
    }
  }
  else
  {
    auto cl_sender = new MyClient(nullptr, "localhost", 1883);
    auto cl_receiver = new MyClient(nullptr, "localhost", 1883);
    if (cl_receiver->subscribe(NULL, "/command", 1) != 0)
    {
      std::cerr << "Error on Mosquitto subscribe topic: /feedback" << std::endl;
      return -1;
    }

    std::cout << "***** Child Process **** CLIENT2 (sender and recevier)  ****** : PID" << getpid() << std::endl;
    std::size_t cnt = 0;
    do
    {
      const auto start{ now() };
      my_msg cmd_recv;
      my_msg feedback_sent;
      int delay = 0;

      cl_sender->publish_with_tracking("/feedback", feedback_sent);

      if (cl_receiver->loop(1) == MOSQ_ERR_SUCCESS)
      {
        if (cl_receiver->getLastReceivedMessage(cmd_recv))
        {
          if (!cl_receiver->isFirstMsgRec())
          {
            std::cout << "CLIENT2  First Message not yet recevied! " << std::endl;
          }
          else
          {
            delay = std::fabs(cl_sender->get_msg_count_cmd() - cmd_recv.msg.count);
            if (delay > maximum_missing_cycle)
            {
              std::cerr << "CLIENT2 delay: " << delay << " exceeds maximum missing cycle ( " << maximum_missing_cycle
                        << " ) . command: " << cl_receiver->get_msg_count_cmd()
                        << ", feedback: " << cmd_recv.msg.count << std::endl;
            }
          }
        }
        else
        {
          std::cerr << "CLIENT2 No new MQTT feedback message available OR first message not received yet... not good, "
                       "topic: /feedback";
        }
      }
      else
      {
        std::cerr << "CLIENT2 loop() failed. check it" << std::endl;
      }

      std::this_thread::sleep_until(awake_time());
      std::chrono::duration<double, std::milli> elapsed{ now() - start };
      if (cnt % 250 == 0)
      {
        std::cout << "CLIENT2 SENT [ms]: " << feedback_sent.msg.count * 4
                  << " LAST RECEIVED  [ms]: " << cmd_recv.msg.count * 4 << "\tDelay [ms]: " << delay * 4
                  << std::endl;
      }
      cnt++;
    } while (true);
  }

  return 0;
}