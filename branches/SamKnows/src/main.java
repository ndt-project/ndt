import skin.*;
import skin.Button;
import skin.Image;
import skin.Label;

import javax.swing.*;
import java.awt.*;

public class main {
    public static void main(String[] args) {

        Interface skin = new Interface();

        skin.setMainBackgroundImage(new Image(new Rectangle(0, 0, 600, 400), "META-INF/background.jpg", true));
        skin.setBackground1Image(new Image(new Rectangle(30, 10, 260, 260), "META-INF/speedbackground.png", true));
//        skin.setBackground2Image(new Image(new Rectangle(310, 10, 260, 260), "META-INF/speedbackground.png", true));
        skin.setValue1Label(new Label(new Rectangle(115, 232, 87, 20), Color.GREEN, new Font("Arial", Font.BOLD, 12), SwingConstants.LEFT, true));
        skin.setValue2Label(new Label(new Rectangle(395, 232, 87, 20), Color.RED, new Font("Arial", Font.BOLD, 12), SwingConstants.CENTER, true));
        skin.setStatusLabel(new Label(new Rectangle(200, 275, 200, 20), Color.CYAN, new Font("Arial", Font.BOLD | Font.ITALIC, 12), SwingConstants.RIGHT, true));
        skin.setRttLabel(new Label(new Rectangle(400, 50, 100, 20), Color.CYAN, new Font("Arial", Font.BOLD | Font.ITALIC, 12), SwingConstants.LEADING, true));
        skin.setLossLabel(new Label(new Rectangle(400, 100, 100, 20), Color.CYAN, new Font("Arial", Font.BOLD | Font.ITALIC, 12), SwingConstants.TRAILING, true));

        skin.setPreparingTestString("Preparing speed test");
        skin.setFirewallTestString("Simple firewall test");
        skin.setUploadTestString("Upload test");
        skin.setDownloadTestString("Download test");
        skin.setCompleteTestString("Complete");

        Button btn = new skin.Button("Start");
        btn.setRect(new Rectangle(100, 300, 120, 50));
        btn.setImageName("META-INF/button1.png");
        btn.setPressedImageName("META-INF/button2.png");
        btn.setTextColor(Color.RED);
        btn.setTextFont(new Font("Arial", Font.ITALIC, 20));
        skin.setStartButton(btn);
        btn = new Button("Stop");
        btn.setRect(new Rectangle(240, 300, 120, 50));
        btn.setImageName("META-INF/button1.png");
        btn.setPressedImageName("META-INF/button2.png");
        btn.setTextColor(Color.GREEN);
        btn.setTextFont(new Font("Arial", Font.ITALIC, 20));
        skin.setStopButton(btn);
        btn = new Button("Advanced");
        btn.setRect(new Rectangle(380, 300, 120, 50));
        btn.setImageName("META-INF/button1.png");
        btn.setPressedImageName("META-INF/button2.png");
        btn.setTextColor(Color.BLUE);
        btn.setTextFont(new Font("Arial", Font.ITALIC, 20));
        skin.setAdvancedButton(btn);

        TextPane pane = new TextPane();
        pane.setX(120);
        pane.setY(350);
        pane.setW(560);
        pane.setH(150);
        skin.setTextPane(pane);

        Arrow arrow = new Arrow();
        arrow.setArrowX(160);
        arrow.setArrowY(140);
        arrow.setArrowAnchorX(5);
        arrow.setArrowAnchorY(5);
        arrow.setFirstAngle(150);
        arrow.setLastAngle(390);
        arrow.setScales(new int[]{0, 50, 100, 300, 500, 1000, 2000, 4000, 8000, 16000, 30000, 50000, 100000});
        arrow.setArrowImageName("META-INF/arrow.png");
        arrow.setVisible(true);
        skin.setArrow1(arrow);

//        arrow = new Arrow();
//        arrow.setArrowX(440);
//        arrow.setArrowY(140);
//        arrow.setArrowAnchorX(5);
//        arrow.setArrowAnchorY(5);
//        arrow.setFirstAngle(150);
//        arrow.setLastAngle(390);
//        arrow.setScales(new int[] {0, 50,  100,  300,   500,  1000,  2000,  4000,  8000, 16000, 30000, 50000, 100000});
//        arrow.setArrowImageName("META-INF/arrow.png");
//        arrow.setVisible(false);
//        skin.setArrow2(arrow);

        skin.save("d:\\interface.properties");

//        SwingUtilities.invokeLater(new Runnable() {
//            public void run() {
//                new MainForm().setVisible(true);
//            }
//        });

    }
}
