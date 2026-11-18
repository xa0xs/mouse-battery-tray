# Adding a mouse

Most wireless mice from Chinese brands come with a **web driver**, a page
that talks to the mouse over WebHID in Chrome or Edge. That page already knows
how to read the battery, so you can copy the protocol from it in a few minutes.
You don't need any reverse-engineering tools.

## 1. Check what the app sees

Right-click the tray icon, choose **Save diagnostics...**. The file lists every
HID collection (VID:PID, usage page, report sizes) and every query that was
tried, with the raw replies. If your mouse is listed but has no level, the
next step finds the protocol.

## 2. Log the web driver

1. Open the web driver (for example the Attack Shark one at
   `https://www.xvalleyinno.top/AttackShark/`), but don't connect yet.
2. Press **F12**, open **Console**, paste [`tools/webhid-logger.js`](../tools/webhid-logger.js)
   and press Enter. It only logs. It does not change anything.
3. Connect the mouse in the page. Lines appear like:

   ```
   sendFeatureReport     My Mouse 8K vvvv:pppp [ff00:1, ...] report 0: 00 00 02 02 00 83 00 00 ...
   receiveFeatureReport  My Mouse 8K vvvv:pppp [...] report 0 -> a1 00 02 02 00 83 00 4b 00 ...
   ```

4. Find the exchange whose reply holds the number the page shows as battery
   (`4b` = 75 here). Often it repeats every few seconds.

## 3. Turn it into a custom device

WebHID leaves the report ID out, but Windows puts it first. So put the report
ID in front of the request, and **add 1** to the reply offsets, unless the
logged reply already starts with the report ID.

From the log above:

| Setting | Value |
|---|---|
| vid | the `vvvv` from the log, e.g. `373E` |
| method | `feature` (it used sendFeatureReport) |
| request | `00 00 00 02 02 00 83` (report ID `00` + data) |
| match | `1=A1,6=83` (bytes that identify the answer) |
| levelByte | `8` (`4b` is at index 7 in the log, + 1) |
| chargingByte | `7` |
| levelScale | `100` |

Use `method = output` when the page used `sendReport` and the answer came as an
`inputreport`. Use `listen` when the battery only arrives as `inputreport`
lines that the page never asked for.

- **Windhawk:** mod settings > Custom devices > Add item.
- **Standalone:** right-click > Settings..., then uncomment the
  `[CustomDevice0]` section and restart the app.

## 4. Share it

Please open an issue or a pull request with the log lines and your settings,
so the mouse can be supported out of the box.
