/* This applet provides an administrator view of the NDT usage
 * statistics.  It was originally developed for the SC-03 conference.
 * An admin script will run to collect the necessary data.  This data
 * is then feed into this applet via the PARM/VALUE pairs in the html
 * file.
 *
 * This applet simply draws the bar graph that appears on the
 * html page.
 *
 * Admin.java by Richard A. Carlson - September 25, 2003
 */

import java.awt.*;
import java.awt.geom.*;
import java.awt.font.*;
import java.awt.image.*;
import java.applet.Applet;

public class Admin extends Applet {

    Color fill, outline, textcolor;
    Font font;
    FontMetrics metrics;
    private String TARGET1 = "Fault";
    private String TARGET2 = "RTT";
    private String TARGET3 = "Dial-up";
    private String TARGET4 = "T1";
    private String TARGET5 = "Enet";
    private String TARGET6 = "T3";
    private String TARGET7 = "FastE";
    private String TARGET8 = "OC-12";
    private String TARGET9 = "GigE";
    private String TARGET10 = "OC-48";
    private String TARGET11 = "tenGE";
    private String TARGET12 = "Total";


    public void init() {

	fill = new Color(0,255,128);	// (RGB values)
	outline = Color.black;

	font = new Font("SansSerif", Font.PLAIN, 12);
	metrics = this.getFontMetrics(font);
    }


    public void paint(Graphics g) {

	int Fault, RTT, Dialup, T1, Enet, T3, FastE;
	int OC12, GigE, OC48, tenGE, Total, i;
	int f;
	String val;

	val = getParameter(TARGET1);
	// System.err.println("Fault = " + val);
	Fault = Integer.parseInt(val);
	val = getParameter(TARGET2);
	// System.err.println("RTT = " + val);
	RTT = Integer.parseInt(val);
	val = getParameter(TARGET3);
	// System.err.println("Dialup = " + val);
	Dialup = Integer.parseInt(val);
	val = getParameter(TARGET4);
	// System.err.println("T1 = " + val);
	T1 = Integer.parseInt(val);
	val = getParameter(TARGET5);
	// System.err.println("Enet = " + val);
	Enet = Integer.parseInt(val);
	val = getParameter(TARGET6);
	// System.err.println("T3 = " + val);
	T3 = Integer.parseInt(val);
	val = getParameter(TARGET7);
	// System.err.println("FastE = " + val);
	FastE = Integer.parseInt(val);
	val = getParameter(TARGET8);
	// System.err.println("OC12 = " + val);
	OC12 = Integer.parseInt(val);
	val = getParameter(TARGET9);
	// System.err.println("GigE = " + val);
	GigE = Integer.parseInt(val);
	val = getParameter(TARGET10);
	// System.err.println("OC48 = " + val);
	OC48 = Integer.parseInt(val);
	val = getParameter(TARGET11);
	// System.err.println("tenGE = " + val);
	tenGE = Integer.parseInt(val);
	val = getParameter(TARGET12);
	Total = Integer.parseInt(val);

	g.setFont(font);

	// draw X/Y axis lines with tick marks on the Y axis
	g.setColor(outline);
	g.drawLine(60, 350, 60, 50);
	g.drawLine(45, 350, 510, 350);
	g.drawString("0", 30, 355);
	g.drawLine(45, 290, 60, 290);
	g.drawString("20", 28, 295);
	g.drawLine(45, 230, 60, 230);
	g.drawString("40", 28, 235);
	g.drawLine(45, 170, 60, 170);
	g.drawString("60", 28, 175);
	g.drawLine(45, 110, 60, 110);
	g.drawString("80", 28, 115);
	g.drawLine(45, 50, 60, 50);
	g.drawString("100", 23, 55);
	
	// draw green boxes representing bars for each possible link type
	f = (int)(((double)Fault/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(70, (349 - f), 30, (1 + f));
	// g.drawString("Fault", 70, 370);
	g.setColor(outline);
	g.drawRect(70, (349 - f), 30, (1 + f));
	g.drawString("Fault", 70, 370);
	if (Fault > 0) {
	    val = "(" + Integer.toString(Fault) + ")";
	    g.drawString(val, (70 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)RTT/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(110, (349 - f), 30, (1 + f));
	// g.drawString("RTT", 110, 385);
	g.setColor(outline);
	g.drawRect(110, (349 - f), 30, (1 + f));
	g.drawString("RTT", 110, 385);
	if (RTT > 0) {
	    val = "(" + Integer.toString(RTT) + ")";
	    g.drawString(val, (110 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)Dialup/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(150, (349 - f), 30, (1 + f));
	// g.drawString("Dial-up", 150, 370);
	g.setColor(outline);
	g.drawRect(150, (349 - f), 30, (1 + f));
	g.drawString("Dial-up", 150, 370);
	if (Dialup > 0) {
	    val = "(" + Integer.toString(Dialup) + ")";
	    g.drawString(val, (150 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)T1/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(190, (349 - f), 30, (1 + f));
	// g.drawString("Cable/DSL", 180, 385);
	g.setColor(outline);
	g.drawRect(190, (349 - f), 30, (1 + f));
	g.drawString("Cable/DSL", 180, 385);
	if (T1 > 0) {
	    val = "(" + Integer.toString(T1) + ")";
	    g.drawString(val, (190 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)Enet/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(230, (349 - f), 30, (1 + f));
	// g.drawString("Ethernet", 230, 370);
	g.setColor(outline);
	g.drawRect(230, (349 - f), 30, (1 + f));
	g.drawString("Ethernet", 230, 370);
	if (Enet > 0) {
	    val = "(" + Integer.toString(Enet) + ")";
	    g.drawString(val, (230 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)T3/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(270, (349 - f), 30, (1 + f));
	// g.drawString("T3/DS3", 270, 385);
	g.setColor(outline);
	g.drawRect(270, (349 - f), 30, (1 + f));
	g.drawString("T3/DS3", 270, 385);
	if (T3 > 0) {
	    i = (int)(((double)T3/300)*(double)Total) + 1; 
	    val = "(" + Integer.toString(T3) + ")";
	    g.drawString(val, (270 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)FastE/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(310, (349 - f), 30, (1 + f));
	// g.drawString("FastEnet", 310, 370);
	g.setColor(outline);
	g.drawRect(310, (349 - f), 30, (1 + f));
	g.drawString("FastEnet", 310, 370);
	if (FastE > 0) {
	    val = "(" + Integer.toString(FastE) + ")";
	    g.drawString(val, (310 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)OC12/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(350, (349 - f), 30, (1 + f));
	// g.drawString("OC-12", 350, 385);
	g.setColor(outline);
	g.drawRect(350, (349 - f), 30, (1 + f));
	g.drawString("OC-12", 350, 385);
	if (OC12 > 0) {
	    val = "(" + Integer.toString(OC12) + ")";
	    g.drawString(val, (360 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)GigE/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(390, (349 - f), 30, (1 + f));
	// g.drawString("GigEnet", 390, 370);
	g.setColor(outline);
	g.drawRect(390, (349 - f), 30, (1 + f));
	g.drawString("GigEnet", 390, 370);
	if (GigE > 0) {
	    val = "(" + Integer.toString(GigE) + ")";
	    g.drawString(val, (390 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)OC48/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(430, (349 - f), 30, (1 + f));
	// g.drawString("OC-48", 430, 385);
	g.setColor(outline);
	g.drawRect(430, (349 - f), 30, (1 + f));
	g.drawString("OC-48", 430, 385);
	if (OC48 > 0) {
	    val = "(" + Integer.toString(OC48) + ")";
	    g.drawString(val, (430 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	f = (int)(((double)tenGE/(double)Total)*300);
	g.setColor(fill);
	g.fillRect(470, (349 - f), 30, (1 + f));
	// g.drawString("10 GigE", 470, 370);
	g.setColor(outline);
	g.drawRect(470, (349 - f), 30, (1 + f));
	g.drawString("10 GigE", 470, 370);
	if (tenGE > 0) {
	    val = "(" + Integer.toString(tenGE) + ")";
	    g.drawString(val, (470 + ((30 - metrics.stringWidth(val))/2)), (339 - f));
	}

	font = new Font("SansSerif", Font.BOLD, 18);
	g.setFont(font);
	g.setColor(outline);
	g.drawString("Bottleneck Link Type (Sub-Total)", 150, 40);
	font = new Font("SansSerif", Font.BOLD, 16);
	g.setFont(font);
	g.drawString("P", 5, 140);
	g.drawString("E", 5, 160);
	g.drawString("R", 5, 180);
	g.drawString("C", 5, 200);
	g.drawString("E", 5, 220);
	g.drawString("N", 5, 240);
	g.drawString("T", 5, 260);
    }

}
