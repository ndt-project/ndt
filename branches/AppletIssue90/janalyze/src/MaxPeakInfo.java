class MaxPeakInfo extends PeakInfo {
    MaxPeakInfo(int cwnd, int sampleRTT) {
        super(cwnd, sampleRTT);
    }

    public String toString() {
        return "max: " + super.toString();
    }
}
