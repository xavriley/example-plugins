MySaw : UGen {
    *ar { arg freq = 440.0, bw = 40.0, mul = 1.0;
        ^this.multiNew('audio', freq, bw, mul)
    }
    *kr { arg freq = 440.0, bw = 40.0, mul = 1.0;
        ^this.multiNew('control', freq, bw, mul)
    }
}
