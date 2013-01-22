package net.measurementlab.ndt;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.Reader;
import java.net.SocketTimeoutException;
import java.security.InvalidParameterException;
import java.util.zip.GZIPInputStream;

import org.apache.http.HeaderElement;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.NameValuePair;
import org.apache.http.ParseException;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.protocol.HTTP;

import org.json.JSONException;
import org.json.JSONObject;

public class MLabNS {

  /** Used by measurement tests if MLabNS should be used to retrieve the real server target. */
  static public final String TARGET = "m-lab";

  /**
   * Query MLab-NS to get an FQDN for the given tool.
   */
  static public String Lookup(Context context, String tool) {
    return Lookup(context, tool, "ipv4", null);
  }

  /**
   * Query MLab-NS to get an FQDN for the given tool and address family.
   */
  static public String Lookup(Context context, String tool, String address_family, String metro) {
    // Set the timeout in milliseconds until a connection is established.
    final int timeoutConnection = 5000;
    // Set the socket timeout in milliseconds.
    final int timeoutSocket = 5000;

    InputStream inputStream = null;

    try {
      // TODO(dominic): This should not be done on the UI thread.
      HttpParams httpParameters = new BasicHttpParams();
      HttpConnectionParams.setConnectionTimeout(httpParameters, timeoutConnection);
      HttpConnectionParams.setSoTimeout(httpParameters, timeoutSocket);
      DefaultHttpClient httpClient = new DefaultHttpClient(httpParameters);
      
      Log.d(NdtSupport.LOG_TAG, "Creating request GET for mlab-ns");

      String url = "http://mlab-ns.appspot.com/" + tool +
          "?format=json&address_family=" + address_family;
      
      // use default policy if metro is null or 'default'
      if ( ! (null == metro || metro.compareTo("default") == 0) ) {
          url += "&policy=metro&metro="+metro;
      }
      HttpGet request = new HttpGet(url);
      request.setHeader("User-Agent", MLabNS.prepareUserAgent(context));

      HttpResponse response = httpClient.execute(request);
      if (response.getStatusLine().getStatusCode() != 200) {
        throw new InvalidParameterException(
            "Received status " + response.getStatusLine().getStatusCode() + " from mlab-ns");
      }
      Log.d(NdtSupport.LOG_TAG, "STATUS OK");

      String body_str = getResponseBody(response);
      JSONObject json = new JSONObject(body_str);
      return String.valueOf(json.getString("fqdn"));
    } catch (SocketTimeoutException e) {
      Log.e(NdtSupport.LOG_TAG, "SocketTimeoutException trying to contact m-lab-ns");
      // e.getMessage() is null       
      throw new InvalidParameterException("Connect to m-lab-ns timeout. Please try again.");
    } catch (IOException e) {
      Log.e(NdtSupport.LOG_TAG, "IOException trying to contact m-lab-ns: " + e.getMessage());
      throw new InvalidParameterException(e.getMessage());
    } catch (JSONException e) {
      Log.e(NdtSupport.LOG_TAG, "JSONException trying to contact m-lab-ns: " + e.getMessage());
      throw new InvalidParameterException(e.getMessage());
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException e) {
          Log.e(NdtSupport.LOG_TAG, "Failed to close the input stream from the HTTP response");
        }
      }
    }
  }

  public static String prepareUserAgent(Context context) {
    // construct User-Agent: use system property 'http.agent' if present, 
    // otherwise return a minimal app_name + user_agent string
    String userAgent = context.getString(R.string.app_name_short) + " ";
    userAgent += System.getProperty("http.agent", 
                                    context.getString(R.string.default_user_agent));
    return userAgent;
  }

  static private String getContentCharSet(final HttpEntity entity) throws ParseException {
    if (entity == null) {
      throw new IllegalArgumentException("entity may not be null");
    }

    String charset = null;
    if (entity.getContentType() != null) {
      HeaderElement values[] = entity.getContentType().getElements();
      if (values.length > 0) {
        NameValuePair param = values[0].getParameterByName("charset");
        if (param != null) {
          charset = param.getValue();
        }
      }
    }
    return charset;
  }

  static private String getResponseBodyFromEntity(HttpEntity entity)
      throws IOException, ParseException {
    if (entity == null) {
      throw new IllegalArgumentException("entity may not be null");
    }

    InputStream instream = entity.getContent();
    if (instream == null) {
      return "";
    }

    if (entity.getContentEncoding() != null) {
      if ("gzip".equals(entity.getContentEncoding().getValue())) {
        instream = new GZIPInputStream(instream);
      }
    }

    if (entity.getContentLength() > Integer.MAX_VALUE) {
      throw new IllegalArgumentException("HTTP entity too large to be buffered into memory");
    }

    String charset = getContentCharSet(entity);
    if (charset == null) {
      charset = HTTP.DEFAULT_CONTENT_CHARSET;
    }

    Reader reader = new InputStreamReader(instream, charset);
    StringBuilder buffer = new StringBuilder();

    try {
      char[] tmp = new char[1024];
      while (reader.read(tmp, 0, tmp.length) != -1) {
        //Log.d(NdtSupport.LOG_TAG, "  reading: " + String.valueOf(tmp));
        buffer.append(tmp);
      }
    } finally {
      reader.close();
    }

    return buffer.toString();
  }

  static private String getResponseBody(HttpResponse response) throws IllegalArgumentException {
    String response_text = null;
    HttpEntity entity = null;

    if (response == null) {
      throw new IllegalArgumentException("response may not be null");
    }

    try {
      entity = response.getEntity();
      response_text = getResponseBodyFromEntity(entity);
    } catch (ParseException e) {
      e.printStackTrace();
    } catch (IOException e) {
      if (entity != null) {
        try {
          entity.consumeContent();
        } catch (IOException e1) {
        }
      }
    }
    return response_text;
  }
}
