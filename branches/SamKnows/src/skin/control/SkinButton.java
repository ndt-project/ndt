package skin.control;

import skin.Button;

import javax.swing.*;
import java.awt.*;

public class SkinButton extends JButton {

    private Button skin;

    public SkinButton(final Button skin, final Insets insets) {
        super();

        this.skin = skin;

        setDoubleBuffered(true);
        setContentAreaFilled(false);
        setBorderPainted(false);
        setFocusPainted(false);
        setOpaque(false);

        final Dimension size = new Dimension(skin.getRect().width, skin.getRect().height);
        setPreferredSize(size);
        setBounds(skin.getRect().x + insets.left, skin.getRect().y + insets.top, size.width, size.height);
    }

    @Override
    protected void paintComponent(final Graphics g) {
        super.paintComponent(g);

        if (model.isPressed())
            g.drawImage(skin.getPressedImage(), 0, 0, null);
        else
            g.drawImage(skin.getImage(), 0, 0, null);

        if (skin.getText() != null) {
            g.setColor(skin.getTextColor());
            g.setFont(skin.getTextFont());
            final String sValue = skin.getText();
            final FontMetrics fm = g.getFontMetrics();
            final int baseline = fm.getMaxAscent() + (getBounds().height - (fm.getAscent() + fm.getMaxDescent())) / 2;
            g.drawString(sValue, (getBounds().width - g.getFontMetrics().stringWidth(sValue)) / 2, baseline);
        }
    }

}
