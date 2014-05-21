package edu.internet2.ndt;

import net.minidev.json.JSONObject;
import net.minidev.json.JSONValue;
import java.util.Iterator;
import java.util.Map;

/**
 * Created by sebastian on 13.05.14.
 */
public class JSONUtils {
    public static final int JSON_SINGLE_VALUE = 1;
    public static final int JSON_MULTIPLE_VALUES = 2;
    public static final int JSON_KEY_VALUE_PAIRS = 3;

    public static String getSingleMessage(String jsonTxt) {
        return getValueFromJsonObj(jsonTxt, "msg");
    }

    public static String getValueFromJsonObj(String jsonTxt, String key) {
        JSONValue jsonParser = new JSONValue();
        Map json = (Map)jsonParser.parse(new String(jsonTxt));
        Iterator iter = json.entrySet().iterator();
        while(iter.hasNext()){
            Map.Entry entry = (Map.Entry)iter.next();
            if (entry.getKey().equals(key)) {
                return entry.getValue().toString();
            }
        }
        return null;
    }

    public static String addValueToJsonObj(String jsonTxt, String key, String value) {
        JSONValue jsonParser = new JSONValue();
        JSONObject json = (JSONObject)jsonParser.parse(new String(jsonTxt));
        json.put(key, value);

        return json.toJSONString();
    }

    public static byte[] createJsonObj(byte[] msg) {
        JSONObject obj = new JSONObject();
        obj.put("msg", new String(msg));

        return obj.toJSONString().getBytes();
    }
}
