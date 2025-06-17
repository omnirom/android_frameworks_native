# Input Device Configuration

There are a number of properties that can be specified for an input device.

|Property|Value|
|---|----|
|`audio.mic`|A boolean (`0` or `1`) that indicates whether the device has a microphone.|
|`device.additionalSysfsLedsNode`|A string representing the path to search for device lights to be used in addition to searching the device node itself for lights.|
|`device.internal`|A boolean (`0` or `1`) that indicates if this input device is part of the device as opposed to be externally attached.|
|`device.type`|A string representing if the device is of a certain type. Valid values include `rotaryEncoder` and `externalStylus`.
