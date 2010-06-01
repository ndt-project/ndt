package skin;

import java.awt.*;
import java.io.Serializable;

public class Label implements Serializable {
    private Rectangle rect;
    private Color textColor;
    private Font textFont;
    private int horizontalAlignment;
    private boolean visible;

    public Label() {
    }

    public Label(final Rectangle rect, final Color textColor, final Font textFont, final int horizontalAlignment, final boolean visible) {
        this.rect = rect;
        this.textColor = textColor;
        this.textFont = textFont;
        this.horizontalAlignment = horizontalAlignment;
        this.visible = visible;
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

    public int getHorizontalAlignment() {
        return horizontalAlignment;
    }

    public void setHorizontalAlignment(final int horizontalAlignment) {
        this.horizontalAlignment = horizontalAlignment;
    }

    public boolean isVisible() {
        return visible;
    }

    public void setVisible(final boolean visible) {
        this.visible = visible;
    }
}