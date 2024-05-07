/**
 *  An interface indicating the balance level of each channel on a stereo device or channel.
 */
export interface VolumeBalance {
    /**
     *  right: The volume for the right channel.
     */
    right: VolumeScalar;
    /**
     *  left: The volume for the left channel.
     */
    left: VolumeScalar;
    /**
     *  stereo: A flag indicating whether the owner is stereo.
     */
    stereo?: boolean;
}
/**
 *  A class that represents an actual physical or virtual audio device.
 */
export declare class Device {
    private constructor();
    /**
     *  The current volume of the device.
     *  @remarks Writing to this property changes the volume of the device.
     */
    volume: VolumeScalar;
    /**
     *  A flag indicating the mute state of a device.
     *  @remarks Writing to this property changes the mute state of the device.
     */
    mute: boolean;
    /**
     *  The stereo balance of the device if available.
     */
    balance: VolumeBalance;
    /**
     *  The name of the device.
     *  @readonly
     */
    readonly name: string;
    /**
     *  The type of the device (input or output).
     *  @readonly
     */
    readonly type: DeviceType;
    /**
     *  Returns the audio sessions bound to the device.
     *  @readonly
     */
    readonly sessions: AudioSession[];
    /**
     *  @param {string} ev - The type of event to subscribe to.
     *  It can be either `volume`, or `mute`.
     *
     *  @param {function} callback - The callback to run when the event is
     *  triggered.
     *
     *  @returns {number} - The id of the registered callback used to
     *  remove the listener.
     *
     *  @remarks Triggering a volume change, or mute change in a callback can
     *  cause the listeners to self trigger, leading to an infinite trigger
     *  loop.
     *
     *  @see {@link Device.removeListener | removing a listener}
     */
    on(ev: string, callback: (payload: any) => void): number;
    /**
     *  @param {string} ev - The type of event to remove the listener of.
     *
     *  @param {number} handler - The identifier of the registered callback
     *  to be removed.
     *
     *  @returns {boolean} - Whether the callback was unregistered or not.
     *
     *  @see {@link Device.on | registering a listener}
     */
    removeListener(ev: string, handler: number): boolean;
}
/**
 *  An enum giving the state of an {@link AudioSession}
 *  @enum
 */
export declare enum AudioSessionState {
    INACTIVE = 0,
    ACTIVE = 1,
    EXPIRED = 2
}
/**
 *  An enum giving the type of {@link Device}, it can be either `input` or
 *  `output`.
 *  @enum
 */
export declare enum DeviceType {
    /**
     *  Input mode.
     */
    CAPTURE = 1,
    /**
     *  Output mode.
     */
    RENDER = 0
}
/**
 *  A floating-point number from `0.0` to `1.0` returning the rate of the
 *  volume. `1.0` represents the most important value, that is, the
 *  {@link Device} or the {@link AudioSession} is at `100%` volume.
 *  `0.0` means that the volume captures or renders no sound at all.
 *  @typedef {number}
 */
export declare type VolumeScalar = number;
/**
 *  A class representing an audio session, that is the sound rendered or
 *  captured by one application.
 *  @class
 */
export declare class AudioSession {
    /**
     *  @private
     */
    private constructor();
    /**
     *  The volume of the {@link AudioSession}
     *  @see {@link Device.volume}
     */
    volume: VolumeScalar;
    /**
     *  The volume balance of the {@link AudioSession} if stereo.
     *  @see {@link Device.balance}.
     */
    balance: VolumeBalance;
    /**
     *  The mute flag of the {@link AudioSession}.
     *  @see {@link Device.mute}.
     */
    mute: boolean;
    /**
     *  The name of the {@link AudioSession}.
     *  @remarks Depending on the `C++` background implementation,
     *  this attribute might not behave the same on all distributions.
     *  @readonly
     */
    readonly name: string;
    /**
     *  The path to the application using the {@link AudioSession}.
     *  @remarks Depending on the `C++` background implementation,
     *  this attribute might not behave the same on all distributions.
     *  @readonly
     */
    readonly appName: string;
    /**
     * The state of the {@link AudioSession}.
     * @readonly
     */
    readonly state: AudioSessionState;
    /**
     * The ID of the {@link AudioSession}.
     * @readonly
     */
    readonly id: string;
}
/**
 *  The sound mixer object containing all
 *  the devices.
 *  @typedef {Object} SoundMixer
 */
export declare type SoundMixer = {
    /**
     *  The list of active {@link Device | devices} when the property is
     *  read.
     *  @static
     */
    devices: Device[];
    /**
     *  Gets the default device of the given type.
     *  @param {DeviceType} type - The type of the device to be retrieved.
     *  @returns {Device} - The default {@link Device} if found, null
     *  otherwise.
     *  @static
     */
    getDefaultDevice(type: DeviceType): Device;
};
/**
 *  The actual {@link SoundMixer} object.
 */
declare const soundMixer: SoundMixer;
export default soundMixer;
