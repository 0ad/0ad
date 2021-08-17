
const NOTIMPLEMENTED = 'Not implemented.';

export class Feature {
    constructor(tf, feature) {
        this.tf = tf;
        this.feature = feature;
        this.enabled = tf[feature];
        this.config = tf.config();
        this.initialized = false;
    }

    init() {
        throw new Error(NOTIMPLEMENTED);
    }

    reset() {
        this.enable();
        this.init();
    }

    destroy() {
        throw new Error(NOTIMPLEMENTED);
    }

    enable() {
        this.enabled = true;
    }

    disable() {
        this.enabled = false;
    }

    isEnabled() {
        return this.enabled;
    }
}
