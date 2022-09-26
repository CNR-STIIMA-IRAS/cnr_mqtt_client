
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

#ifndef __CNR_MQTT_CLIENT__
#define __CNR_MQTT_CLIENT__

#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mosquitto.h>


namespace cnr
{
  namespace mqtt
  {
    class MsgDecoder
    {
    public:
      MsgDecoder(): data_valid_(false), new_msg_available_(false) { };
      // The method should be reimplemented on the base of the application
      virtual void on_message( struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg ) { };
      bool isDataValid() {return data_valid_; };
      bool isNewMessageAvailable() { return new_msg_available_;}
      void setDataValid(const bool& data_valid) {data_valid_ = data_valid;}      
      void setNewMessageAvailable(const bool& new_msg_available) {new_msg_available_ = new_msg_available;}

    private:
      bool data_valid_;
      bool new_msg_available_;
    };

    class MsgEncoder
    {
    public:
      MsgEncoder() { };
      // The method should be reimplemented on the base of the application
      virtual void on_publish(struct mosquitto *mosq, void *obj, int mid) { };
    };

    class MQTTClient 
    {
    private: 
      struct mosquitto *mosq_;
      uint8_t obj_[1024];
      int stop_raised_ = 0; 
      char errbuffer_[1024] = {0};
      bool mosq_initialized_ = false;

    public:
      MQTTClient() = delete;
      MQTTClient( const char *id, const char *host, int port, MsgEncoder* msg_encoder, MsgDecoder* msg_decoder);
      //MQTTClient (const char *id, const char *host, int port );
      ~MQTTClient();

      int loop();
      int stop() {return stop_raised_ = 1;}

      int reconnect(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff);
      int subscribe(int *mid, const char *sub, int qos=0);
      int unsubscribe(int *mid, const char *sub);
      int publish(const void* payload, int& payload_len, const char* topic_name);

      typedef void (MQTTClient::*on_connect_callback)  (struct mosquitto *mosq, void *obj, int reason_code);
      typedef void (MQTTClient::*on_subscribe_callback)(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
      typedef void (MQTTClient::*on_publish_callback)  (struct mosquitto *mosq, void *obj, int mid);
      typedef void (MQTTClient::*on_message_callback)  (struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
     
      void on_connect  (struct mosquitto *mosq, void *obj, int reason_code);
      void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos); 
      void on_publish  (struct mosquitto *mosq, void *obj, int mid);
      void on_message  (struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

    };

    bool init_library(std::shared_ptr<MsgEncoder> msg_encoder, std::shared_ptr<MsgDecoder> msg_decoder );
  } // end mqtt namespace 
} // end cnr namespace

#endif //__CNR_MQTT_CLIENT__
