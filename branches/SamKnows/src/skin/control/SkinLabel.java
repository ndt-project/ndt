package skin.control;

import skin.Label;

import javax.swing.*;
import java.awt.*;


public class SkinLabel extends JLabel {

//    LEFT     SwingConstants.LEFT
//    CENTER   SwingConstants.CENTER

    //    RIGHT    SwingConstants.RIGHT
//    LEADING  SwingConstants.LEADING
//    TRAILING SwingConstants.TRAILING
    public SkinLabel(final Label skin, final Insets insets) {
        super();

        final Dimension size = new Dimension(skin.getRect().width, skin.getRect().height);
        setPreferredSize(size);
        setBounds(skin.getRect().x + insets.left, skin.getRect().y + insets.top,
                size.width, size.height);

        setForeground(skin.getTextColor());
        setFont(skin.getTextFont());
        setVisible(skin.isVisible());
        setHorizontalAlignment(skin.getHorizontalAlignment());
    }

}