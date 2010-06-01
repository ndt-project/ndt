package skin;

import javax.imageio.ImageIO;
import java.beans.XMLDecoder;
import java.beans.XMLEncoder;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.Serializable;

public class Interface implements Serializable {

    private static Interface iface;
    private final static String fileName = "/META-INF/interface.xml";

    private String preparingTestString;
    private String firewallTestString;
    private String uploadTestString;
    private String downloadTestString;
    private String completeTestString;
    private String waitString;
    private String stopString;

    private Image mainBackgroundImage;
    private Image background1Image;
    private Image background2Image;
    private Button startButton;
    private Button stopButton;
    private Button advancedButton;
    private TextPane textPane;
    private Arrow arrow1;
    private Arrow arrow2;
    private Label statusLabel;
    private Label value1Label;
    private Label value2Label;
    private Label rttLabel;
    private Label lossLabel;

    public String getPreparingTestString() {
        return preparingTestString;
    }

    public void setPreparingTestString(final String preparingTestString) {
        this.preparingTestString = preparingTestString;
    }

    public String getFirewallTestString() {
        return firewallTestString;
    }

    public void setFirewallTestString(final String firewallTestString) {
        this.firewallTestString = firewallTestString;
    }

    public String getUploadTestString() {
        return uploadTestString;
    }

    public void setUploadTestString(final String uploadTestString) {
        this.uploadTestString = uploadTestString;
    }

    public String getDownloadTestString() {
        return downloadTestString;
    }

    public void setDownloadTestString(final String downloadTestString) {
        this.downloadTestString = downloadTestString;
    }

    public String getCompleteTestString() {
        return completeTestString;
    }

    public void setCompleteTestString(final String completeTestString) {
        this.completeTestString = completeTestString;
    }

    public String getWaitString() {
        return waitString;
    }

    public void setWaitString(final String waitString) {
        this.waitString = waitString;
    }

    public String getStopString() {
        return stopString;
    }

    public void setStopString(final String stopString) {
        this.stopString = stopString;
    }

    public Image getMainBackgroundImage() {
        return mainBackgroundImage;
    }

    public void setMainBackgroundImage(final Image mainBackgroundImage) {
        this.mainBackgroundImage = mainBackgroundImage;
    }

    public Image getBackground1Image() {
        return background1Image;
    }

    public void setBackground1Image(final Image background1Image) {
        this.background1Image = background1Image;
    }

    public Image getBackground2Image() {
        return background2Image;
    }

    public void setBackground2Image(final Image background2Image) {
        this.background2Image = background2Image;
    }

    public Button getStartButton() {
        return startButton;
    }

    public void setStartButton(final Button startButton) {
        this.startButton = startButton;
    }

    public Button getStopButton() {
        return stopButton;
    }

    public void setStopButton(final Button stopButton) {
        this.stopButton = stopButton;
    }

    public Button getAdvancedButton() {
        return advancedButton;
    }

    public void setAdvancedButton(final Button advancedButton) {
        this.advancedButton = advancedButton;
    }

    public TextPane getTextPane() {
        return textPane;
    }

    public void setTextPane(final TextPane textPane) {
        this.textPane = textPane;
    }

    public Arrow getArrow1() {
        return arrow1;
    }

    public void setArrow1(final Arrow arrow1) {
        this.arrow1 = arrow1;
    }

    public Arrow getArrow2() {
        return arrow2;
    }

    public void setArrow2(final Arrow arrow2) {
        this.arrow2 = arrow2;
    }

    public Label getStatusLabel() {
        return statusLabel;
    }

    public void setStatusLabel(final Label statusLabel) {
        this.statusLabel = statusLabel;
    }

    public Label getValue1Label() {
        return value1Label;
    }

    public void setValue1Label(final Label value1Label) {
        this.value1Label = value1Label;
    }

    public Label getValue2Label() {
        return value2Label;
    }

    public void setValue2Label(final Label value2Label) {
        this.value2Label = value2Label;
    }

    public Label getRttLabel() {
        return rttLabel;
    }

    public void setRttLabel(final Label rttLabel) {
        this.rttLabel = rttLabel;
    }

    public Label getLossLabel() {
        return lossLabel;
    }

    public void setLossLabel(final Label lossLabel) {
        this.lossLabel = lossLabel;
    }

    public void save(final String fileName) {
        try {
            final FileOutputStream fos = new FileOutputStream(fileName);
            final XMLEncoder encoder = new XMLEncoder(fos);
            encoder.writeObject(this);
            encoder.close();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    public static Interface getInstance() {
        if (iface == null) {
            try {
                final InputStream is = Interface.class.getResourceAsStream(fileName);
                final XMLDecoder decoder = new XMLDecoder(is);
                iface = (Interface) decoder.readObject();
                decoder.close();

                iface.startButton.setImage(loadImage(iface.startButton.getImageName()));
                iface.startButton.setPressedImage(loadImage(iface.startButton.getPressedImageName()));
                iface.stopButton.setImage(loadImage(iface.stopButton.getImageName()));
                iface.stopButton.setPressedImage(loadImage(iface.stopButton.getPressedImageName()));
                iface.advancedButton.setImage(loadImage(iface.advancedButton.getImageName()));
                iface.advancedButton.setPressedImage(loadImage(iface.advancedButton.getPressedImageName()));

                iface.mainBackgroundImage.setImage(loadImage(iface.mainBackgroundImage.getImageName()));
                if (iface.background1Image != null && iface.background1Image.isVisible())
                    iface.background1Image.setImage(loadImage(iface.background1Image.getImageName()));
                if (iface.background2Image != null && iface.background2Image.isVisible())
                    iface.background2Image.setImage(loadImage(iface.background2Image.getImageName()));
                if (iface.arrow1 != null && iface.arrow1.isVisible())
                    iface.arrow1.setArrowImage(loadImage(iface.arrow1.getArrowImageName()));
                if (iface.arrow2 != null && iface.arrow2.isVisible())
                    iface.arrow2.setArrowImage(loadImage(iface.arrow2.getArrowImageName()));
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
        return iface;
    }

    private static java.awt.Image loadImage(final String path) throws IOException {
        java.awt.Image img = null;
        final InputStream in = Interface.class.getClassLoader().getResourceAsStream(path);
        if (in != null) {
            img = ImageIO.read(in);
            in.close();
        }
        return img;
    }

}
