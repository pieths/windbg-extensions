// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

"use strict";

function __BoolToIntString(value) {
    return value ? "1" : "0";
}

class __Base_FilePath_Visualizer
{
    toString() {
        return this.path_.toString();
    }
}

class __Base_UnguessableToken_Visualizer
{
    toString() {
        return "{" + this.token_.words_[0].toString() + ", "
                   + this.token_.words_[1].toString() + "}";
    }
}

class __Blink_WebMediaKeySystemMediaCapability_Visualizer
{
    toString() {
        let encryption_scheme = [
            "kNotSpecified",
            "kCenc",
            "kCbcs",
            "kCbcs_1_9",
            "kUnrecognized"
        ][this.encryption_scheme];

        let displayString =
            "{" + this.content_type.toString() + ", "
                + this.robustness.toString() + ","
                + " Encrypt: " + encryption_scheme + "}";
        displayString = displayString.trim().replace(/\s\s+/g, ' ');
        return displayString;
    }
}

class __Content_CdmInfo_Visualizer
{
    toString() {
        let robustness = [
            "kHardwareSecure",
            "kSoftwareSecure"
        ][this.robustness];

        let status = [
            "kUninitialized",
            "kEnabled",
            "kCommandLineOverridden",
            "kHardwareSecureDecryptionDisabled",
            "kAcceleratedVideoDecodeDisabled",
            "kGpuFeatureDisabled",
            "kGpuCompositionDisabled",
            "kDisabledByPref",
            "kDisabledOnError",
            "kDisabledBySoftwareEmulatedGpu"
        ][this.status];

        let displayString =
            "{" + this.key_system.toString() + ", "
                + robustness + ","
                + " Status: " + status + ","
                + " SupportsSubKeySystems: " + __BoolToIntString(this.supports_sub_key_systems) + "}";
        displayString = displayString.trim().replace(/\s\s+/g, ' ');
        return displayString;
    }
}

class __Media_CdmConfig_Visualizer
{
    toString() {
        return [
            this.key_system.toString(),
            "AllowDI: " + __BoolToIntString(this.allow_distinctive_identifier),
            "AllowPS: " + __BoolToIntString(this.allow_persistent_state),
            "UseHwSecureCodecs: " + __BoolToIntString(this.use_hw_secure_codecs)
        ].join(" ");
    }
}

function initializeScript()
{
    return [
        new host.typeSignatureRegistration(__Base_FilePath_Visualizer, "base::FilePath"),
        new host.typeSignatureRegistration(__Base_UnguessableToken_Visualizer, "base::UnguessableToken"),
        new host.typeSignatureRegistration(__Blink_WebMediaKeySystemMediaCapability_Visualizer, "blink::WebMediaKeySystemMediaCapability"),
        new host.typeSignatureRegistration(__Content_CdmInfo_Visualizer, "content::CdmInfo"),
        new host.typeSignatureRegistration(__Media_CdmConfig_Visualizer, "media::CdmConfig"),
    ];
}
