class PeakInfo {
    private int cwnd;
    private int sampleRTT;

    PeakInfo(int cwnd, int sampleRTT) {
        this.cwnd = cwnd;
        this.sampleRTT = sampleRTT;
    }

    int getCwnd() {
        return cwnd;
    }

    int getSampleRTT() {
        return sampleRTT;
    }

    public String toString() {
        return "Cwnd = " + cwnd + ", sampleRTT = " + sampleRTT + ", " + getPeakSpeed(cwnd, sampleRTT);
    }

    public static String getPeakSpeed(int cwnd, int sampleRTT) {
        double pSpeed = (cwnd * 8.0) / sampleRTT;
        if (pSpeed < 1000.0) {
            return "peak speed = " + Helpers.formatDouble(pSpeed, 4) + " kbps";
        }
        else if (pSpeed < 1000000.0) {
            return "peak speed = " + Helpers.formatDouble(pSpeed / 1000.0, 4) + " Mbps";
        }
        else {
            return "peak speed = " + Helpers.formatDouble(pSpeed / 1000000.0, 4) + " Gbps";
        }

    }

    public static double getPeakSpeedInMbps(int cwnd, int sampleRTT) {
        double pSpeed = (cwnd * 8.0) / sampleRTT;
        return pSpeed / 1000.0;
    }
}
