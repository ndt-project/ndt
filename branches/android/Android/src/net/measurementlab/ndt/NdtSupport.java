package net.measurementlab.ndt;

import java.util.HashMap;
import java.util.Map;

import android.app.Activity;
import android.graphics.Typeface;
import android.widget.TextView;

/**
 * Mixin to share code useful across all parts of the app.
 * 
 * @author gideon@newamerica.net
 * 
 */
class NdtSupport {
	static final String DEFAULT_TYPEFACE = "fonts/League_Gothic.otf";

	private static Map<String, Typeface> typefaces = new HashMap<String, Typeface>();

	/**
	 * Convenience that assumes a specific default font should be uniformaly
	 * applied.
	 * 
	 * @see NdtSupport#applyFont(Activity, String, int...)
	 */
	static void applyFont(Activity activity, int... textFieldIds) {
		applyFont(activity, DEFAULT_TYPEFACE, textFieldIds);
	}

	/**
	 * Convenience for setting a whole array of text fields to the same
	 * typeface.
	 * 
	 * @param activity
	 *            used to find the fields to alter
	 * @param typefaceResource
	 *            the resource path to the typeface of interest
	 * @param textFieldIds
	 *            vararg of ids to look up and change
	 */
	static void applyFont(Activity activity, String typefaceResource,
			int... textFieldIds) {
		Typeface typeface = null;
		if (typefaces.containsKey(typefaceResource)) {
			typeface = typefaces.get(typefaceResource);
		} else {
			typeface = Typeface.createFromAsset(activity.getAssets(),

			"fonts/League_Gothic.otf");
			typefaces.put(typefaceResource, typeface);
		}
		for (int textFieldId : textFieldIds) {
			TextView textView = (TextView) activity.findViewById(textFieldId);
			textView.setTypeface(typeface);
		}
	}

	/**
	 * TAG constant for logging.
	 */
	static final String LOG_TAG = "NDT";
}
