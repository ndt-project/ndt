
public class Helpers {
	public static String formatDouble(double val, int precision) {
		String format_string = "%1$." + precision + "f";
		return String.format(format_string, val);
	}
}