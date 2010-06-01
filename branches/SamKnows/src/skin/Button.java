package skin;

import java.awt.*;
import java.io.Serializable;

public class Button implements Serializable {
    private Rectangle rect;

    private Color textColor;
    private Font textFont;

    private String text;
    private String imageName;
    private String pressedImageName;

    private java.awt.Image image;
    private java.awt.Image pressedImage;

    public Button() {
    }

    public Button(final String text) {
        this.text = text;
    }

    public Rectangle getRect() {
        return rect;
    }

    public void setRect(final Rectangle rect) {
        this.rect = rect;
    }

    public Color getTextColor() {
        return textColor;
    }

    public void setTextColor(final Color textColor) {
        this.textColor = textColor;
    }

    public Font getTextFont() {
        return textFont;
    }

    public void setTextFont(final Font textFont) {
        this.textFont = textFont;
    }

    public String getText() {
        return text;
    }

    public void setText(final String text) {
        this.text = text;
    }

    public String getImageName() {
        return imageName;
    }

    public void setImageName(final String imageName) {
        this.imageName = imageName;
    }

    public String getPressedImageName() {
        return pressedImageName;
    }

    public void setPressedImageName(final String pressedImageName) {
        this.pressedImageName = pressedImageName;
    }

    public java.awt.Image getImage() {
        return image;
    }

    public void setImage(final java.awt.Image image) {
        this.image = image;
    }

    public java.awt.Image getPressedImage() {
        return pressedImage;
    }

    public void setPressedImage(final java.awt.Image pressedImage) {
        this.pressedImage = pressedImage;
    }
}
