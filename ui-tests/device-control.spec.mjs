import { expect, test } from "@playwright/test";

test("@device control flow sanity on live rig UI", async ({ page, request }) => {
  const apiResp = await request.get("/api/network");
  if (!apiResp.ok()) {
    test.skip(true, "Backend API not available at /api/network");
  }

  await page.goto("/", { waitUntil: "domcontentloaded" });
  await page.click("#tabControl");

  await expect(page.locator("#btnStart")).toBeVisible();
  await expect(page.locator("#btnSnap")).toBeVisible();
  await expect(page.locator("#manualLastAction")).toBeVisible();

  // Preset selection and apply path (non-destructive parameter update path).
  await page.click("#presetSmall");
  await page.click("#presetMedium");
  await expect(page.locator("#presetMedium")).toHaveClass(/selected/);
  if (!(await page.locator("#btnApplyPreset").isDisabled())) {
    await page.click("#btnApplyPreset");
  }

  // UI-only toggles should always react.
  const triggerTextBefore = await page.locator("#btnTriggerToggle").innerText();
  await page.click("#btnTriggerToggle");
  const triggerTextAfter = await page.locator("#btnTriggerToggle").innerText();
  expect(triggerTextAfter).not.toEqual(triggerTextBefore);

  // Snapshot button action when available should update last-action label.
  const snapBtn = page.locator("#btnSnap");
  if (!(await snapBtn.isDisabled())) {
    await snapBtn.click();
    await expect(page.locator("#manualLastAction")).toContainText(/SNAP requested/i);
  }

  // If TT is disconnected, START must show warning.
  const ttText = (await page.locator("#ttBadge").innerText()).toUpperCase();
  if (ttText.includes("DISCONNECTED")) {
    await page.click("#btnStart");
    await expect(page.locator("#startWarn")).toContainText(/Turntable not connected/i);
  }
});
