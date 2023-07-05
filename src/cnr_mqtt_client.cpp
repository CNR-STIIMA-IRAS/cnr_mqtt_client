/*
 *  Software License Agreement (New BSD License)
 *
 *  Copyright 2022 National Council of Research of Italy (CNR)
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

#include <stdexcept>
#include <vector>

#include <cnr_mqtt_client/cnr_mqtt_client.h>
#include <cnr_mqtt_client/dynamic_callback.h>

#define MAX_NUM_ENC_DEC 10
namespace cnr
{
namespace mqtt
{
template <typename T>
void maybe_unused(T const& v)
{
  static_cast<void>(v);
}

template <typename T, typename... Args>
void maybe_unused(T first, Args... args)
{
  maybe_unused(first);
  return maybe_unused(args...);
}

int enc_dec_counter = 0;
MsgEncoder* g_msg_encoder[MAX_NUM_ENC_DEC];
MsgDecoder* g_msg_decoder[MAX_NUM_ENC_DEC];

int init_library(MsgEncoder* msg_encoder, MsgDecoder* msg_decoder)
{
  if (enc_dec_counter < MAX_NUM_ENC_DEC)
  {
    g_msg_encoder[enc_dec_counter] = msg_encoder;
    g_msg_decoder[enc_dec_counter] = msg_decoder;
    return enc_dec_counter++;
  }
  return -1;
}

MsgDecoder::MsgDecoder() : data_valid_(false), new_msg_available_(false)
{
}
void MsgDecoder::on_message(const struct mosquitto_message* msg)
{
  maybe_unused(msg);
}
bool MsgDecoder::isDataValid()
{
  return data_valid_;
}
bool MsgDecoder::isNewMessageAvailable()
{
  return new_msg_available_;
}
void MsgDecoder::setDataValid(const bool& data_valid)
{
  data_valid_ = data_valid;
}
void MsgDecoder::setNewMessageAvailable(const bool& new_msg_available)
{
  new_msg_available_ = new_msg_available;
}

void MsgEncoder::on_publish(int mid)
{
  maybe_unused(mid);
}

MQTTClient::MQTTClient(const char* id, const char* host, int port, MsgEncoder* msg_encoder, MsgDecoder* msg_decoder)
{
  if (msg_encoder && msg_decoder == NULL)
  {
    std::cout << "NULL ptr to encoder and/or decoder library" << std::endl;
    return;
  }

  if (cnr::mqtt::init_library(msg_encoder, msg_decoder) < 0)
  {
    std::cout << "Cannot initialize the encoder and decoder library" << std::endl;
    return;
  }

  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();

  /* Create a new client instance.
   * id = NULL -> ask the broker to generate a client id for us
   * obj = NULL -> we aren't passing any of our private data for callbacks
   */
  mosq_ = mosquitto_new(id, true, obj_);
  if (mosq_ == NULL)
  {
    printf("Error: Out of memory.");
    throw std::runtime_error("Error: Out of memory.");
  }

  /* Configure callbacks. This should be done before connecting ideally. */
  mosquitto_connect_callback_set(mosq_, OnConnectMemberFunctionCallback(this, &MQTTClient::on_connect));
  mosquitto_subscribe_callback_set(mosq_, OnSubscribeMemberFunctionCallback(this, &MQTTClient::on_subscribe));
  mosquitto_message_callback_set(mosq_, OnMessageMemberFunctionCallback(this, &MQTTClient::on_message));
  mosquitto_publish_callback_set(mosq_, OnPublishMemberFunctionCallback(this, &MQTTClient::on_publish));

  int rc = mosquitto_connect(mosq_, host, port, 600);
  if (rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq_);
#ifdef _WIN32
    strerror_s(errbuffer_, 1024, rc);
    printf("Error while connecting to the broker: %s", errbuffer_);
#else
    printf("Error while connecting to the broker: %s", strerror_r(rc, errbuffer_, 1024));
#endif
    throw std::runtime_error("Error while connecting to MQTT broker.");
  }

  mosq_initialized_ = true;
}

MQTTClient::~MQTTClient()
{
  if (mosq_initialized_)
  {
    mosquitto_lib_cleanup();
  }
}

int MQTTClient::loop(int timeout)
{
  if (mosq_initialized_)
  {
    int rc = 0;
    /* Run the network loop in a background thread, this call returns quickly. */
    rc = mosquitto_loop(mosq_, timeout, 1);
    if (rc != MOSQ_ERR_SUCCESS)
    {
#ifdef _WIN32
      strerror_s(errbuffer_, 1024, rc);
      printf("Error in loop: %s", errbuffer_);
#else
      printf("Error in loop: %s", strerror_r(rc, errbuffer_, 1024));
#endif
    }
    return rc;
  }

  throw std::runtime_error("Mosquitto not initialized.");
}

int MQTTClient::reconnect()
{
  if (mosq_initialized_)
  {
    int rc = mosquitto_reconnect(mosq_);
    if (rc != MOSQ_ERR_SUCCESS)
    {
#ifdef _WIN32
      strerror_s(errbuffer_, 1024, rc);
      printf("Error on subscribe: %s", errbuffer_);
#else
      printf("Error on subscribe: %s", strerror_r(rc, errbuffer_, 1024));
#endif
    }
    return rc;
  }

  throw std::runtime_error("Mosquitto not initialized.");
}

int MQTTClient::subscribe(int* mid, const char* sub, int qos)
{
  if (mosq_initialized_)
  {
    int rc = mosquitto_subscribe(mosq_, mid, sub, qos);
    if (rc != MOSQ_ERR_SUCCESS)
    {
#ifdef _WIN32
      strerror_s(errbuffer_, 1024, rc);
      printf("Error on subscribe: %s", errbuffer_);
#else
      printf("Error on subscribe: %s", strerror_r(rc, errbuffer_, 1024));
#endif
    }
    return rc;
  }

  throw std::runtime_error("Mosquitto not initialized.");
}

int MQTTClient::unsubscribe(int* mid, const char* sub)
{
  if (mosq_initialized_)
  {
    int rc = mosquitto_unsubscribe(mosq_, mid, sub);

    if (rc != MOSQ_ERR_SUCCESS)
    {
#ifdef _WIN32
      strerror_s(errbuffer_, 1024, rc);
      printf("Error on unsubscribe: %s", errbuffer_);
#else
      printf("Error on unsubscribe: %s", strerror_r(rc, errbuffer_, 1024));
#endif
    }

    return rc;
  }

  throw std::runtime_error("Mosquitto not initialized.");
}

int MQTTClient::publish(const void* payload, const int& payload_len, const char* topic_name)
{
  if (mosq_initialized_)
  {
    int rc = mosquitto_publish(mosq_, NULL, topic_name, payload_len, payload, 0, false);

    if (rc != MOSQ_ERR_SUCCESS)
    {
#ifdef _WIN32
      strerror_s(errbuffer_, 1024, rc);
      printf("Error on publish: %s", errbuffer_);
#else
      printf("Error on publish: %s", strerror_r(rc, errbuffer_, 1024));
#endif
    }

    return rc;
  }

  throw std::runtime_error("Mosquitto not initialized.");
}

void MQTTClient::on_connect(struct mosquitto* mosq, void* obj, int reason_code)
{
  maybe_unused(obj);
  if (reason_code != 0)
  {
    mosquitto_disconnect(mosq);
  }
}

void MQTTClient::on_subscribe(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos)
{
  maybe_unused(obj);
  maybe_unused(mid);
  bool have_subscription = false;
  for (int i = 0; i < qos_count; i++)
  {
    if (granted_qos[i] <= 2)
      have_subscription = true;
  }

  if (have_subscription == false)
    mosquitto_disconnect(mosq);
}

void MQTTClient::on_publish(int index, struct mosquitto* mosq, void* obj, int mid)
{
  maybe_unused(mosq);
  maybe_unused(obj);
  g_msg_encoder[index]->on_publish(mid);
}

void MQTTClient::on_message(int index, struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg)
{
  maybe_unused(mosq);
  maybe_unused(obj);
  g_msg_decoder[index]->on_message(msg);
}

}  // end namespace mqtt
}  // end namespace cnr
