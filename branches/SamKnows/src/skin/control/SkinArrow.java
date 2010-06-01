package skin.control;

import skin.Arrow;

import javax.swing.*;
import java.awt.*;
import java.awt.geom.AffineTransform;
import java.awt.image.AffineTransformOp;
import java.awt.image.BufferedImage;
import java.awt.image.BufferedImageOp;

public class SkinArrow {

    private Arrow skin;

    private int value = 0;

    private double firstAngle;
    private double intervalAngle;
    private double[] angles;
    private MoveArrow task = null;

    private JPanel parent;

    public SkinArrow(final JPanel parent, final Arrow skin) {
        this.parent = parent;
        this.skin = skin;

        firstAngle = Math.toRadians(skin.getFirstAngle());
        final double lastAngle = Math.toRadians(skin.getLastAngle());
        intervalAngle = (lastAngle - firstAngle) / (skin.getScales().length - 1);

        angles = new double[skin.getScales().length];
        for (int i = 1; i < skin.getScales().length; i++) {
            angles[i] = angles[i - 1] + intervalAngle;
        }
    }

    protected class MoveArrow extends SwingWorker<Exception, String> {
        int newValue;
        int count;
        int interval;

        public MoveArrow(final int oldValue, final int newValue) {
            value = oldValue;
            this.newValue = newValue;
            interval = (newValue - oldValue) / 10;
            count = 0;
        }

        protected Exception doInBackground() throws Exception {
            while (count < 10) {
                value += interval;
                parent.repaint();
                count++;
                Thread.sleep(500 / 10);
            }
            if (value != newValue) {
                value = newValue;
                parent.repaint();
            }
            return null;
        }
    }

    public void setValue(final int value) {
        if (value >= 0 && value <= skin.getScales()[skin.getScales().length - 1]) {
            if (task != null) {
                task.cancel(true);
                task = null;
            }
            if (Math.abs(this.value - value) > 200) {
                task = new MoveArrow(this.value, value);
                task.execute();
            } else {
                this.value = value;
                parent.repaint();
            }
        }
    }

    private double getAngle() {
        double value1 = value;
        double firstAngle1 = firstAngle;
        for (int i = 1; i < skin.getScales().length; i++) {
            if (value <= skin.getScales()[i]) {
                firstAngle1 = firstAngle + angles[i - 1];
                value1 = (value - skin.getScales()[i - 1]) * intervalAngle / (skin.getScales()[i] - skin.getScales()[i - 1]);
                break;
            }
        }
        return value1 + firstAngle1;
    }

    // draw arrow
    public void drawArrow(final Graphics g) {
        final double angle = getAngle();

        final Graphics2D g2d = (Graphics2D) g;
        final AffineTransform origXform = g2d.getTransform();
        final AffineTransform newXform = (AffineTransform) origXform.clone();
        //center of rotation is center of the panel
        newXform.rotate(angle, skin.getArrowX(), skin.getArrowY());
        g2d.setTransform(newXform);
        //draw image centered in panel
        final BufferedImageOp bio = new AffineTransformOp(new AffineTransform(), AffineTransformOp.TYPE_BILINEAR);
        g2d.drawImage((BufferedImage) skin.getArrowImage(), bio, skin.getArrowX() - skin.getArrowAnchorX(), skin.getArrowY() - skin.getArrowAnchorY());
        g2d.setTransform(origXform);
    }

}
