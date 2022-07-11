# Energy Example

This example shows how to use the Blues Wireless Notecard library to send and receive notes periodically. The example puts the Notecard in `minimum` [mode](https://dev.blues.io/reference/notecard-api/hub-requests/#hub-set) where data is synced between the Notecard and Notehub only when the code requests it via a `hub.sync` request.

Before running the example, please set the `PRODUCT_UID`. Details on what a `PRODUCT_UID` is and how to set it can be found at [https://dev.blues.io/tools-and-sdks/samples/product-uid](https://dev.blues.io/tools-and-sdks/samples/product-uid).