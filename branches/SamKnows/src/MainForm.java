import skin.Interface;
import skin.control.SkinButton;
import skin.control.SkinPanel;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Random;

public class MainForm extends JDialog {

    private static Random rn = new Random();

    public static int rand(int lo, int hi) {
        int n = hi - lo + 1;
        int i = rn.nextInt() % n;
        if (i < 0)
            i = -i;
        return lo + i;
    }

    public MainForm() {

        Interface skin = Interface.getInstance();

        final SkinPanel contentPane = new SkinPanel(skin);
        contentPane.setLayout(null);

        SkinButton btn = contentPane.getStartButton();

        setContentPane(contentPane);

        Insets insets = this.getInsets();
        setSize(600 + insets.left + insets.right,
                400 + insets.top + insets.bottom);


        btn.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                contentPane.setValue1(rand(0, 100000));
                contentPane.setValue2(rand(0, 100000));
            }
        });


        setLocationRelativeTo(null);

        setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);

    }

}
