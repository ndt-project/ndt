package skin;

import java.io.Serializable;

public class Arrow implements Serializable {
    private int arrowX;
    private int arrowY;
    private int arrowAnchorX;
    private int arrowAnchorY;
    private int firstAngle;
    private int lastAngle;
    private int[] scales;
    private String arrowImageName;
    private java.awt.Image arrowImage;
    private boolean visible;

    public Arrow() {
    }

    public int getArrowX() {
        return arrowX;
    }

    public void setArrowX(final int arrowX) {
        this.arrowX = arrowX;
    }

    public int getArrowY() {
        return arrowY;
    }

    public void setArrowY(final int arrowY) {
        this.arrowY = arrowY;
    }

    public int getArrowAnchorX() {
        return arrowAnchorX;
    }

    public void setArrowAnchorX(final int arrowAnchorX) {
        this.arrowAnchorX = arrowAnchorX;
    }

    public int getArrowAnchorY() {
        return arrowAnchorY;
    }

    public void setArrowAnchorY(final int arrowAnchorY) {
        this.arrowAnchorY = arrowAnchorY;
    }

    public int getFirstAngle() {
        return firstAngle;
    }

    public void setFirstAngle(final int firstAngle) {
        this.firstAngle = firstAngle;
    }

    public int getLastAngle() {
        return lastAngle;
    }

    public void setLastAngle(final int lastAngle) {
        this.lastAngle = lastAngle;
    }

    public int[] getScales() {
        return scales;
    }

    public void setScales(final int[] scales) {
        this.scales = scales;
    }

    public String getArrowImageName() {
        return arrowImageName;
    }

    public void setArrowImageName(final String arrowImageName) {
        this.arrowImageName = arrowImageName;
    }

    public java.awt.Image getArrowImage() {
        return arrowImage;
    }

    public void setArrowImage(final java.awt.Image arrowImage) {
        this.arrowImage = arrowImage;
    }

    public boolean isVisible() {
        return visible;
    }

    public void setVisible(final boolean visible) {
        this.visible = visible;
    }
}