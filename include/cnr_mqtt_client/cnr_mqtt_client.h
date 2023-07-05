
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

#include <mutex>
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
      MsgDecoder();
      MsgDecoder(const MsgDecoder&) = delete;
      MsgDecoder(MsgDecoder&&) = delete;
      virtual ~MsgDecoder() = default;
      // The method should be reimplemented on the base of the application
      //virtual void on_message( struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg ) { };
      virtual void on_message( const struct mosquitto_message *msg );
      bool isDataValid();
      bool isNewMessageAvailable();
      void setDataValid(const bool& data_valid);
      void setNewMessageAvailable(const bool& new_msg_available);

    private:
      bool data_valid_;
      bool new_msg_available_;
    
    public:
      std::timed_mutex mtx_;
    };

    class MsgEncoder
    {
    public:
      MsgEncoder() = default;
      MsgEncoder(const MsgEncoder&) = delete;
      MsgEncoder(MsgEncoder&&) = delete;
      virtual ~MsgEncoder() = default;
      // The method should be reimplemented on the base of the application
      virtual void on_publish( int mid);
    };

    int init_library(MsgEncoder* msg_encoder, MsgDecoder* msg_decoder );

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
      virtual ~MQTTClient();

      int loop(int timeout=2000);
      int stop() {return stop_raised_ = 1;}

      int reconnect();
      int subscribe(int *mid, const char *sub, int qos=0);
      int unsubscribe(int *mid, const char *sub);
      int publish(const void* payload, const int& payload_len, const char* topic_name);

      typedef void (MQTTClient::*on_connect_callback)  (struct mosquitto *mosq, void *obj, int reason_code);
      typedef void (MQTTClient::*on_subscribe_callback)(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
      typedef void (MQTTClient::*on_publish_callback)  (int index, struct mosquitto *mosq, void *obj, int mid);
      typedef void (MQTTClient::*on_message_callback)  (int index, struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
     
      void on_connect  (struct mosquitto *mosq, void *obj, int reason_code);
      void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos); 
      void on_publish  (int index, struct mosquitto *mosq, void *obj, int mid);
      void on_message  (int index, struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
    };

  } // end mqtt namespace 
} // end cnr namespace

#endif //__CNR_MQTT_CLIENT__
