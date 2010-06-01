package skin;

import java.io.Serializable;

public class TextPane implements Serializable {
    private int x;
    private int y;
    private int w;
    private int h;

    public TextPane() {
    }

    public int getX() {
        return x;
    }

    public void setX(final int x) {
        this.x = x;
    }

    public int getY() {
        return y;
    }

    public void setY(final int y) {
        this.y = y;
    }

    public int getW() {
        return w;
    }

    public void setW(final int w) {
        this.w = w;
    }

    public int getH() {
        return h;
    }

    public void setH(final int h) {
        this.h = h;
    }

}