package skin;

import java.awt.*;
import java.io.Serializable;

public class Image implements Serializable {
    private Rectangle rect;
    private String imageName;
    private java.awt.Image image;
    private boolean visible;

    public Image() {
    }

    public Image(final Rectangle rect, final String imageName, final boolean visible) {
        this.rect = rect;
        this.imageName = imageName;
        this.visible = visible;
    }

    public Rectangle getRect() {
        return rect;
    }

    public void setRect(final Rectangle rect) {
        this.rect = rect;
    }

    public String getImageName() {
        return imageName;
    }

    public void setImageName(final String imageName) {
        this.imageName = imageName;
    }

    public java.awt.Image getImage() {
        return image;
    }

    public void setImage(final java.awt.Image image) {
        this.image = image;
    }

    public boolean isVisible() {
        return visible;
    }

    public void setVisible(final boolean visible) {
        this.visible = visible;
    }
}