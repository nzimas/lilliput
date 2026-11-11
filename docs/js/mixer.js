/* SPDX-License-Identifier: MIT -- Lilliput website: the Concept page's desk, one routing at a time */
(function () {
  var fig = document.querySelector(".mixer");
  if (!fig) return;
  var bar = fig.querySelector(".mixer-routes"), name = fig.querySelector(".mixer-name"),
      cap = fig.querySelector(".mixer-cap"), btns = bar.querySelectorAll("button"),
      groups = fig.querySelectorAll("g[data-p]");
  bar.hidden = false;
  bar.addEventListener("click", function (e) {
    var b = e.target.closest("button");
    if (!b) return;
    var p = b.getAttribute("data-p");
    groups.forEach(function (g) {
      if (g.getAttribute("data-p") === p) g.removeAttribute("display"); else g.setAttribute("display", "none");
    });
    btns.forEach(function (x) { x.setAttribute("aria-pressed", x === b ? "true" : "false"); });
    name.textContent = b.textContent;
    cap.textContent = b.getAttribute("data-cap");
  });
})();
