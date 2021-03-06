//--------------------------------------------------------------------------
// Copyright (C) 2019-2020 Cisco and/or its affiliates. All rights reserved.
//--------------------------------------------------------------------------

// appid_app_descriptor.cc author Shravan Rangaraju <shrarang@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "appid_app_descriptor.h"
#include "app_info_table.h"
#include "appid_config.h"
#include "appid_module.h"
#include "appid_peg_counts.h"
#include "appid_types.h"
#include "lua_detector_api.h"

using namespace snort;

void ApplicationDescriptor::set_id(AppId app_id)
{
    if ( my_id != app_id )
    {
        my_id = app_id;
        if ( app_id > APP_ID_NONE )
            update_stats(app_id);
        else if ( app_id == APP_ID_UNKNOWN )
            appid_stats.appid_unknown++;
        else
            return; // app_id == APP_ID_NONE

        if ( overwritten_id > APP_ID_NONE )
        {
            update_stats(overwritten_id, false);
            overwritten_id = APP_ID_NONE;
        }
    }
}

void ApplicationDescriptor::set_id(const Packet& p, AppIdSession& asd,
    AppidSessionDirection dir, AppId app_id, AppidChangeBits& change_bits)
{
    if ( my_id != app_id )
    {
        set_id(app_id);
        check_detector_callback(p, asd, dir, app_id, change_bits);
    }
}

void ServiceAppDescriptor::update_stats(AppId id, bool increment)
{
    AppIdPegCounts::update_service_count(id, increment);
}

void ServiceAppDescriptor::set_port_service_id(AppId id)
{
    if ( id != port_service_id )
    {
        port_service_id = id;
        if ( id > APP_ID_NONE )
            AppIdPegCounts::update_service_count(id, true);
    }
}

void ServiceAppDescriptor::set_id(AppId app_id, OdpContext& odp_ctxt)
{
    if (get_id() != app_id)
    {
        ApplicationDescriptor::set_id(app_id);
        deferred = odp_ctxt.get_app_info_mgr().get_app_info_flags(app_id, APPINFO_FLAG_DEFER);
    }
}

void ClientAppDescriptor::update_user(AppId app_id, const char* username)
{
    if ( my_username != username )
        my_username = username;

    if ( my_user_id != app_id )
    {
        my_user_id = app_id;
        if ( app_id > APP_ID_NONE )
            AppIdPegCounts::inc_user_count(app_id);
    }
}

void ClientAppDescriptor::update_stats(AppId id, bool increment)
{
    AppIdPegCounts::update_client_count(id, increment);
}

void PayloadAppDescriptor::update_stats(AppId id, bool increment)
{
    AppIdPegCounts::update_payload_count(id, increment);
}
