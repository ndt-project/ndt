package edu.internet2.ndt;

class MinPeakInfo extends PeakInfo {
    MinPeakInfo(int cwnd, int sampleRTT) {
        super(cwnd, sampleRTT);
    }

    public String toString() {
        return "min: " + super.toString();
    }
}
