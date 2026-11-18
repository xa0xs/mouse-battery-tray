// WebHID logger: paste into the DevTools console (F12) of a mouse's web
// driver page BEFORE you click "Connect". It only logs what the page sends
// and receives; it changes nothing. Then watch the battery indicator of the
// page: the report that produces it is your mouse's battery protocol.
//
// Byte offsets: WebHID leaves the report ID out of the data, Windows (and
// this app's settings) put it in front as byte 0. So add 1 to offsets you
// read here, and put the report ID before the request bytes.
(() => {
  const hex = (view) => {
    const u8 = view instanceof DataView
      ? new Uint8Array(view.buffer, view.byteOffset, view.byteLength)
      : new Uint8Array(view.buffer ?? view, view.byteOffset ?? 0, view.byteLength);
    return Array.from(u8, (b) => b.toString(16).padStart(2, "0")).join(" ");
  };
  const id = (d) =>
    `${d.productName} ${d.vendorId.toString(16).padStart(4, "0")}:` +
    `${d.productId.toString(16).padStart(4, "0")} [` +
    d.collections.map((c) => `${c.usagePage.toString(16)}:${c.usage.toString(16)}`).join(", ") +
    "]";
  const P = HIDDevice.prototype;
  const wrap = (name, describe) => {
    const original = P[name];
    P[name] = async function (...args) {
      const result = await original.apply(this, args);
      console.log(`%c${name}`, "color:#39f;font-weight:bold", id(this), describe(args, result));
      return result;
    };
  };
  wrap("sendFeatureReport", (a) => `report ${a[0]}: ${hex(a[1])}`);
  wrap("sendReport", (a) => `report ${a[0]}: ${hex(a[1])}`);
  wrap("receiveFeatureReport", (a, r) => `report ${a[0]} -> ${hex(r)}`);

  const log = (e) =>
    console.log("%cinputreport", "color:#e80;font-weight:bold", id(e.device),
                `report ${e.reportId}: ${hex(e.data)}`);
  const desc = Object.getOwnPropertyDescriptor(P, "oninputreport");
  Object.defineProperty(P, "oninputreport", {
    configurable: true,
    get() { return desc.get.call(this); },
    set(fn) {
      desc.set.call(this, fn && ((e) => { log(e); return fn.call(this, e); }));
    },
  });
  const addEventListener = P.addEventListener;
  P.addEventListener = function (type, fn, ...rest) {
    if (type === "inputreport") {
      const inner = fn;
      fn = (e) => { log(e); return typeof inner === "function" ? inner(e) : inner.handleEvent(e); };
    }
    return addEventListener.call(this, type, fn, ...rest);
  };
  console.log("WebHID logger ready: connect the mouse now.");
})();
