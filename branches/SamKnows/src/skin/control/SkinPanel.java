package skin.control;

import skin.Interface;

import javax.swing.*;
import java.awt.*;

public class SkinPanel extends JPanel {

    private Interface skin;

    private SkinLabel statusLabel;
    private SkinButton startButton;
    private SkinButton stopButton;
    private SkinButton advancedButton;
    private SkinLabel value1Label;
    private SkinLabel value2Label;
    private SkinLabel rttLabel;
    private SkinLabel lossLabel;
    private SkinArrow arrow1;
    private SkinArrow arrow2;

    public SkinPanel(final Interface skin) {
        super();
        this.skin = skin;
        setLayout(null);

        setDoubleBuffered(true);
        setOpaque(false);

        Insets insets = getInsets();

        statusLabel = new SkinLabel(skin.getStatusLabel(), insets);
        add(statusLabel);

        startButton = new SkinButton(skin.getStartButton(), insets);
        add(startButton);
        stopButton = new SkinButton(skin.getStopButton(), insets);
        add(stopButton);
        advancedButton = new SkinButton(skin.getAdvancedButton(), insets);
        add(advancedButton);

        value1Label = new SkinLabel(skin.getValue1Label(), insets);
        add(value1Label);
        if (skin.getValue2Label() != null && skin.getValue2Label().isVisible()) {
            value2Label = new SkinLabel(skin.getValue2Label(), insets);
            add(value2Label);
        }
        rttLabel = new SkinLabel(skin.getRttLabel(), insets);
        add(rttLabel);
        lossLabel = new SkinLabel(skin.getLossLabel(), insets);
        add(lossLabel);

        arrow1 = new SkinArrow(this, skin.getArrow1());
        if (skin.getArrow2() != null && skin.getArrow2().isVisible())
            arrow2 = new SkinArrow(this, skin.getArrow2());

    }

    protected void paintComponent(final Graphics g) {
        if (skin.getMainBackgroundImage().isVisible()) {
            g.drawImage(skin.getMainBackgroundImage().getImage(),
                    skin.getMainBackgroundImage().getRect().x,
                    skin.getMainBackgroundImage().getRect().y,
                    skin.getMainBackgroundImage().getRect().width,
                    skin.getMainBackgroundImage().getRect().height,
                    null);
        }
        if (skin.getBackground1Image() != null && skin.getBackground1Image().isVisible()) {
            g.drawImage(skin.getBackground1Image().getImage(),
                    skin.getBackground1Image().getRect().x,
                    skin.getBackground1Image().getRect().y,
                    skin.getBackground1Image().getRect().width,
                    skin.getBackground1Image().getRect().height,
                    null);
        }
        if (skin.getBackground2Image() != null && skin.getBackground2Image().isVisible()) {
            g.drawImage(skin.getBackground2Image().getImage(),
                    skin.getBackground2Image().getRect().x,
                    skin.getBackground2Image().getRect().y,
                    skin.getBackground2Image().getRect().width,
                    skin.getBackground2Image().getRect().height,
                    null);
        }
        arrow1.drawArrow(g);
        if (skin.getArrow2() != null && skin.getArrow2().isVisible())
            arrow2.drawArrow(g);
    }

    public void setValue1(final int value) {
        if (value > 100000)
            return;
        value1Label.setText(String.format("%.2f", value / 1000.0) + " Mb/s");
        arrow1.setValue(value);
    }

    public void setValue1_(final int value) {
        if (value > 100000)
            return;
        arrow1.setValue(value);
    }

    public void setValue2(final int value) {
        if (value > 100000)
            return;
        if (skin.getValue2Label() != null && skin.getValue2Label().isVisible())
            value2Label.setText(String.format("%.2f", value / 1000.0) + " Mb/s");
        else
            value1Label.setText(String.format("%.2f", value / 1000.0) + " Mb/s");
        if (skin.getArrow2() != null && skin.getArrow2().isVisible())
            arrow2.setValue(value);
        else
            arrow1.setValue(value);
    }

    public void setValue2_(final int value) {
        if (value > 100000)
            return;
        if (skin.getArrow2() != null && skin.getArrow2().isVisible())
            arrow2.setValue(value);
        else
            arrow1.setValue(value);
    }

    public void setRtt(final double value) {
        rttLabel.setText(String.format("%.2f", value));
    }

    public void setLoss(final double value) {
        lossLabel.setText(String.format("%.2f", value * 100));
    }

    public SkinLabel getStatusLabel() {
        return statusLabel;
    }

    public SkinButton getStartButton() {
        return startButton;
    }

    public SkinButton getStopButton() {
        return stopButton;
    }

    public SkinButton getAdvancedButton() {
        return advancedButton;
    }
}
