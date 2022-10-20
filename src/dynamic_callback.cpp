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

#include <cnr_mqtt_client/dynamic_callback.h>
#include <cnr_mqtt_client/cnr_mqtt_client.h>


namespace cnr
{
  namespace mqtt
  {
    OnConnectCallbackBase* AvailableOnConnectCallbackSlots[] = {
        new OnConnectDynamicCallback<0x00>(), 
        new OnConnectDynamicCallback<0x01>(), 
        new OnConnectDynamicCallback<0x02>(), 
        new OnConnectDynamicCallback<0x03>(), 
        new OnConnectDynamicCallback<0x04>(), 
        new OnConnectDynamicCallback<0x05>(), 
        new OnConnectDynamicCallback<0x06>(), 
        new OnConnectDynamicCallback<0x07>(), 
        new OnConnectDynamicCallback<0x08>(), 
        new OnConnectDynamicCallback<0x09>(), 
        new OnConnectDynamicCallback<0x0A>(), 
    };
    OnMessageCallbackBase* AvailableOnMessageCallbackSlots[] = {
        new OnMessageDynamicCallback<0x00>(), 
        new OnMessageDynamicCallback<0x01>(), 
        new OnMessageDynamicCallback<0x02>(), 
        new OnMessageDynamicCallback<0x03>(), 
        new OnMessageDynamicCallback<0x04>(), 
        new OnMessageDynamicCallback<0x05>(), 
        new OnMessageDynamicCallback<0x06>(), 
        new OnMessageDynamicCallback<0x07>(), 
        new OnMessageDynamicCallback<0x08>(), 
        new OnMessageDynamicCallback<0x09>(), 
        new OnMessageDynamicCallback<0x0A>(), 
    };
    OnSubscribeCallbackBase* AvailableOnSubscribeCallbackSlots[] = {
        new OnSubscribeDynamicCallback<0x00>(), 
        new OnSubscribeDynamicCallback<0x01>(), 
        new OnSubscribeDynamicCallback<0x02>(), 
        new OnSubscribeDynamicCallback<0x03>(), 
        new OnSubscribeDynamicCallback<0x04>(), 
        new OnSubscribeDynamicCallback<0x05>(), 
        new OnSubscribeDynamicCallback<0x06>(), 
        new OnSubscribeDynamicCallback<0x07>(), 
        new OnSubscribeDynamicCallback<0x08>(), 
        new OnSubscribeDynamicCallback<0x09>(), 
        new OnSubscribeDynamicCallback<0x0A>(), 
    };
    OnPublishCallbackBase* AvailableOnPublishCallbackSlots[] = {
        new OnPublishDynamicCallback<0x00>(), 
        new OnPublishDynamicCallback<0x01>(), 
        new OnPublishDynamicCallback<0x02>(), 
        new OnPublishDynamicCallback<0x03>(), 
        new OnPublishDynamicCallback<0x04>(), 
        new OnPublishDynamicCallback<0x05>(), 
        new OnPublishDynamicCallback<0x06>(), 
        new OnPublishDynamicCallback<0x07>(), 
        new OnPublishDynamicCallback<0x08>(), 
        new OnPublishDynamicCallback<0x09>(), 
        new OnPublishDynamicCallback<0x0A>(), 
    };

    // ==============
    OnConnectCallbackBase::OnConnectCallbackBase(on_connect_callback pCCallback) : 
    m_pClass( NULL ),m_pMethod( NULL ),m_pCCallback( pCCallback )
    {
    }
    void OnConnectCallbackBase::Free()
    {
        m_pClass = NULL;
    }


    on_connect_callback OnConnectCallbackBase::Reserve(MQTTClient* instance, MQTTClient::on_connect_callback method)
    { 
        if( m_pClass )
            return NULL;

        if( m_pMethod != NULL )
          return NULL;
        
        m_pClass = instance;
        
        m_pMethod = method;
        return m_pCCallback;
    }

    void OnConnectCallbackBase::StaticInvoke(int context, struct mosquitto *mosq, void *obj, int reason_code)
    {
        auto p = (AvailableOnConnectCallbackSlots[context]->m_pClass);
        return (p->*(AvailableOnConnectCallbackSlots[context]->m_pMethod)) (mosq, obj, reason_code);
    }

    // ==============
    OnMessageCallbackBase::OnMessageCallbackBase(on_message_callback pCCallback) : 
    m_pClass( NULL ),m_pMethod( NULL ),m_pCCallback( pCCallback )
    {
    }
    void OnMessageCallbackBase::Free()
    {
        m_pClass = NULL;
    }


    on_message_callback OnMessageCallbackBase::Reserve(MQTTClient* instance, MQTTClient::on_message_callback method)
    { 
        if( m_pClass )
            return NULL;
        
        if( m_pMethod != NULL )
          return NULL;
        
        m_pClass = instance;
        m_pMethod = method;
        return m_pCCallback;
    }

    void OnMessageCallbackBase::StaticInvoke(int context, struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
    {
        auto p = (AvailableOnMessageCallbackSlots[context]->m_pClass); 
        return (p->*(AvailableOnMessageCallbackSlots[context]->m_pMethod)) (mosq, obj, msg);
    }

    // ==============
    OnSubscribeCallbackBase::OnSubscribeCallbackBase(on_subscribe_callback pCCallback) : 
    m_pClass( NULL ),m_pMethod( NULL ),m_pCCallback( pCCallback )
    {
    }
    void OnSubscribeCallbackBase::Free()
    {
        m_pClass = NULL;
    }


    on_subscribe_callback OnSubscribeCallbackBase::Reserve(MQTTClient* instance, MQTTClient::on_subscribe_callback method)
    { 
        if( m_pClass )
            return NULL;
        
        if( m_pMethod != NULL )
          return NULL;
        
        m_pClass = instance;
        m_pMethod = method;
        return m_pCCallback;
    }

    void OnSubscribeCallbackBase::StaticInvoke(int context, struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
    {
        auto p = (AvailableOnSubscribeCallbackSlots[context]->m_pClass);
        return (p->*(AvailableOnSubscribeCallbackSlots[context]->m_pMethod)) (mosq, obj, mid, qos_count, granted_qos);
    }

    // ==============
    OnPublishCallbackBase::OnPublishCallbackBase(on_publish_callback pCCallback) : 
    m_pClass( NULL ),m_pMethod( NULL ),m_pCCallback( pCCallback )
    {
    }
    void OnPublishCallbackBase::Free()
    {
        m_pClass = NULL;
    }

    on_publish_callback OnPublishCallbackBase::Reserve(MQTTClient* instance, MQTTClient::on_publish_callback method)
    {
        if( m_pClass )
            return NULL;
        
        if( m_pMethod != NULL )
          return NULL;
        
        m_pClass = instance;
        m_pMethod = method;
        return m_pCCallback;
    }

    void OnPublishCallbackBase::StaticInvoke(int context, struct mosquitto *mosq, void *obj, int mid)
    {
        auto p = (AvailableOnPublishCallbackSlots[context]->m_pClass);
        return (p->*(AvailableOnPublishCallbackSlots[context]->m_pMethod)) (mosq, obj, mid);
    }

    // ***********
    bool OnConnectMemberFunctionCallback::IsValid() const
    {
        return m_cbCallback != NULL;
    }

    OnConnectMemberFunctionCallback::OnConnectMemberFunctionCallback(MQTTClient* instance, MQTTClient::on_connect_callback method)
    {
        int imax = sizeof(AvailableOnConnectCallbackSlots)/sizeof(AvailableOnConnectCallbackSlots[0]);
        for( m_nAllocIndex = 0; m_nAllocIndex < imax; ++m_nAllocIndex )
        {
          m_cbCallback = AvailableOnConnectCallbackSlots[m_nAllocIndex]->Reserve(instance, method);
          if( m_cbCallback != NULL )
              break;
        }
    }
    OnConnectMemberFunctionCallback::~OnConnectMemberFunctionCallback()
    {
        if( IsValid() )
        {
        AvailableOnConnectCallbackSlots[m_nAllocIndex]->Free();
        }
    }

    // ***********
    bool OnMessageMemberFunctionCallback::IsValid() const
    {
        return m_cbCallback != NULL;
    }

    OnMessageMemberFunctionCallback::OnMessageMemberFunctionCallback(MQTTClient* instance, MQTTClient::on_message_callback method)
    {
        int imax = sizeof(AvailableOnMessageCallbackSlots)/sizeof(AvailableOnMessageCallbackSlots[0]);
        for( m_nAllocIndex = 0; m_nAllocIndex < imax; ++m_nAllocIndex )
        {
        m_cbCallback = AvailableOnMessageCallbackSlots[m_nAllocIndex]->Reserve(instance, method);
        if( m_cbCallback != NULL )
            break;
        }
    }

    OnMessageMemberFunctionCallback::~OnMessageMemberFunctionCallback()
    {
        if( IsValid() )
        {
        AvailableOnMessageCallbackSlots[m_nAllocIndex]->Free();
        }
    }

    // ***********
    bool OnSubscribeMemberFunctionCallback::IsValid() const
    {
        return m_cbCallback != NULL;
    }

    OnSubscribeMemberFunctionCallback::OnSubscribeMemberFunctionCallback(MQTTClient* instance, MQTTClient::on_subscribe_callback method)
    {
        int imax = sizeof(AvailableOnSubscribeCallbackSlots)/sizeof(AvailableOnSubscribeCallbackSlots[0]);
        for( m_nAllocIndex = 0; m_nAllocIndex < imax; ++m_nAllocIndex )
        {
        m_cbCallback = AvailableOnSubscribeCallbackSlots[m_nAllocIndex]->Reserve(instance, method);
        if( m_cbCallback != NULL )
            break;
        }
    }
    OnSubscribeMemberFunctionCallback::~OnSubscribeMemberFunctionCallback()
    {
        if( IsValid() )
        {
        AvailableOnSubscribeCallbackSlots[m_nAllocIndex]->Free();
        }
    }

    // ******************************************************************
    bool OnPublishMemberFunctionCallback::IsValid() const
    {
        return m_cbCallback != NULL;
    }

    OnPublishMemberFunctionCallback::OnPublishMemberFunctionCallback(MQTTClient* instance, MQTTClient::on_publish_callback method)
    {
        int imax = sizeof(AvailableOnPublishCallbackSlots)/sizeof(AvailableOnPublishCallbackSlots[0]);
        for( m_nAllocIndex = 0; m_nAllocIndex < imax; ++m_nAllocIndex )
        {
        m_cbCallback = AvailableOnPublishCallbackSlots[m_nAllocIndex]->Reserve(instance, method);
        if( m_cbCallback != NULL )
            break;
        }
    }
    OnPublishMemberFunctionCallback::~OnPublishMemberFunctionCallback()
    {
        if( IsValid() )
        {
            AvailableOnPublishCallbackSlots[m_nAllocIndex]->Free();
        }
    }
  }
} // end namespace cnr
