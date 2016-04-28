package com.gagara.homekeeper.nbi.response;

import org.json.JSONObject;

public interface Response {

    Response fromJson(JSONObject json);

}
