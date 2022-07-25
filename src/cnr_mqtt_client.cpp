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

#include <iostream>
#include <stdexcept>
#include <vector>

#include <cnr_mqtt_client/cnr_mqtt_client.h>
#include <cnr_mqtt_client/dynamic_callback.h>

namespace cnr
{  
  namespace mqtt
  {

    MQTTClient::MQTTClient( const char *id, const char *host, int port, MsgDecoder *msg_decoder, MsgEncoder *msg_encoder):
                            decoder_(msg_decoder), encoder_(msg_encoder)
    {
      /* Required before calling other mosquitto functions */
      mosquitto_lib_init();

      /* Create a new client instance.
      * id = NULL -> ask the broker to generate a client id for us
      * obj = NULL -> we aren't passing any of our private data for callbacks
      */
      mosq_ = mosquitto_new(id, true, obj_);
      if(mosq_ == NULL)
      {
        printf("Error: Out of memory.");
        throw std::runtime_error("Error: Out of memory.");
      }

      /* Configure callbacks. This should be done before connecting ideally. */
      mosquitto_connect_callback_set(mosq_, OnConnectMemberFunctionCallback(this, &MQTTClient::on_connect));
      mosquitto_subscribe_callback_set(mosq_, OnSubscribeMemberFunctionCallback(this, &MQTTClient::on_subscribe));
      mosquitto_message_callback_set(mosq_, OnMessageMemberFunctionCallback(this, &MQTTClient::on_message));
      mosquitto_publish_callback_set(mosq_, OnPublishMemberFunctionCallback(this, &MQTTClient::on_publish));

      /* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
      * This call makes the socket connection only, it does not complete the MQTT
      * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
      * mosquitto_loop_forever() for processing net traffic. */

      int rc = mosquitto_connect(mosq_, host, port, 60);
      if( rc != MOSQ_ERR_SUCCESS )
      {
        mosquitto_destroy(mosq_);
        //printf("Error: %s ", strerror_r(rc,errbuffer_,1024) );
        strerror_s(errbuffer_, 1024, rc);
        printf("Error: %s", errbuffer_ );
        throw std::runtime_error("Error");
      }
    }

    MQTTClient::~MQTTClient()
    {
      /* Run the network loop in a blocking call. The only thing we do in this
      * example is to print incoming messages, so a blocking call here is fine.
      *
      * This call will continue forever, carrying automatic reconnections if
      * necessary, until the user calls mosquitto_disconnect().
      */      
      mosquitto_lib_cleanup();
    }

    int MQTTClient::loop()
    {
      /* Run the network loop in a blocking call. The only thing we do in this
      * example is to print incoming messages, so a blocking call here is fine.
      *
      * This call will continue forever, carrying automatic reconnections if
      * necessary, until the user calls mosquitto_disconnect().
      */
      // return mosquitto_loop_forever(mosq, -1, 1);
      
      int rc = 0;
      /* Run the network loop in a background thread, this call returns quickly. */
      rc = mosquitto_loop(mosq_,2000,1);
      if(rc != MOSQ_ERR_SUCCESS)
      {
        mosquitto_destroy(mosq_);
        //printf("Error: %s", strerror_r(rc, errbuffer_, 1024) );
        strerror_s(errbuffer_, 1024, rc);
        printf("Error: %s", errbuffer_ );
        return -1;
      }

      /* At this point the client is connected to the network socket, but may not
      * have completed CONNECT/CONNACK.
      * It is fairly safe to start queuing messages at this point, but if you
      * want to be really sure you should wait until after a successful call to
      * the connect callback.
      * In this case we know it is 1 second before we start publishing.
      */

      return rc;
    }

    int MQTTClient::reconnect(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
    {
      return -1; //mosquitto_reconnect_delay_set(mosq,reconnect_delay, reconnect_delay_max, reconnect_exponential_backoff);
    }
        
    int MQTTClient::subscribe(int *mid, const char *sub, int qos)
    {
      /* Making subscriptions in the on_connect() callback means that if the
      * connection drops and is automatically resumed by the client, then the
      * subscriptions will be recreated when the client reconnects. */
      int rc = mosquitto_subscribe(mosq_, mid, sub, qos);
      if(rc != MOSQ_ERR_SUCCESS)
      {
        //printf("Error on topic subscription: %s", strerror_r(rc, errbuffer_,1024) );
        strerror_s(errbuffer_, 1024, rc);
        printf("Error: %s", errbuffer_ );
        /* We might as well disconnect if we were unable to subscribe */
        mosquitto_disconnect(mosq_);
        return -1;
      }
      return rc;
    }


    int MQTTClient::unsubscribe(int *mid, const char *sub)
    {
      /* Unsubscribe from the client, then the
      * subscriptions will be recreated when the client reconnects. */
      int rc = mosquitto_unsubscribe(mosq_, mid, sub);

      if( rc != MOSQ_ERR_SUCCESS )
      {
        strerror_s(errbuffer_,1024,rc);
        printf("Error: %s", errbuffer_ );
        return -1;
      }
      
      return rc;
    }


    /* This function pretends to read some data from a sensor and publish it.*/
    int MQTTClient::publish( const uint8_t* payload, const uint32_t& payload_len, const std::string& topic_name )
    {
      /* Publish the message
      * mosq - our client instance 
      * *mid = NULL - we don't want to know what the message id for this message is
      * topic = "example/temperature" - the topic on which this message will be published
      * payloadlen = strlen(payload) - the length of our payload in bytes
      * payload - the actual payload
      * qos = 2 - publish with QoS 2 for this example
      * retain = false - do not use the retained message feature for this message
      */
      int rc = mosquitto_publish(mosq_, NULL, topic_name.c_str(), payload_len, payload, 0, false);
      if( rc != MOSQ_ERR_SUCCESS )
      {
        strerror_s(errbuffer_,1024,rc);
        printf("Error: %s", errbuffer_ );
        return -1;
      }
      
      return rc;
    }

    void MQTTClient::on_connect(struct mosquitto *mosq, void *obj, int reason_code)
    {
      /* Print out the connection result. mosquitto_connack_string() produces an
      * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
      * clients is mosquitto_reason_string().
      */
      if(reason_code != 0)
      {
        /* If the connection fails for any reason, we don't want to keep on
        * retrying in this example, so disconnect. Without this, the client
        * will attempt to reconnect. */
        mosquitto_disconnect(mosq);
      }
    }

    void MQTTClient::on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
    {
      bool have_subscription = false;
      /*  In this example we only subscribe to a single topic at once, but a
          SUBSCRIBE can contain many topics at once, so this is one way to check them all. */
      for( int i=0; i<qos_count; i++)
      {
        if(granted_qos[i] <= 2)
          have_subscription = true;
      }

      if( have_subscription == false )
      {
        /* The broker rejected all of our subscriptions, we know we only sent
          the one SUBSCRIBE, so there is no point remaining connected. */
        mosquitto_disconnect(mosq);
      }
    }


    void MQTTClient::on_publish(struct mosquitto *mosq, void *obj, int mid)
    {
      encoder_->on_publish();
    }

    void MQTTClient::on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
    {
      decoder_->on_message( msg );
    }

  } // end namespace mqtt
} // end namespace cnr
