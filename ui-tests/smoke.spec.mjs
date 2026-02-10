import { expect, test } from "@playwright/test";

function parseBadgeState(text) {
  const v = String(text || "").toUpperCase();
  if (v.includes("CONNECTED") || v.includes("ON")) return "on";
  if (v.includes("DISCONNECTED") || v.includes("OFF")) return "off";
  return "unknown";
}

test("renders UI and runs core interaction smoke checks", async ({ page, baseURL }) => {
  await page.goto(baseURL || "/", { waitUntil: "domcontentloaded" });

  await expect(page.locator("header .brand")).toHaveText("RealityScanRig3000");
  await expect(page.locator("#tabControl")).toBeVisible();
  await expect(page.locator("#tabSettings")).toBeVisible();
  await expect(page.locator("#connBadge")).toBeVisible();
  await expect(page.locator("#ttBadge")).toBeVisible();

  await page.click("#tabSettings");
  await expect(page.locator("#setWifiSsid")).toBeVisible();
  await expect(page.locator("#btnNetReload")).toBeVisible();
  await expect(page.locator("#btnNetSave")).toBeVisible();
  await expect(page.locator("#btnNetSaveReboot")).toBeVisible();
  await expect(page.locator("#settingsDirtyHint")).toBeVisible();

  // Layout sanity: ensure dirty hint is above save row with some gap.
  const dirtyBox = await page.locator("#settingsDirtyHint").boundingBox();
  const saveRowBox = await page.locator("#btnNetSave").boundingBox();
  expect(dirtyBox).not.toBeNull();
  expect(saveRowBox).not.toBeNull();
  if (dirtyBox && saveRowBox) {
    const gap = saveRowBox.y - (dirtyBox.y + dirtyBox.height);
    expect(gap).toBeGreaterThanOrEqual(6);
  }

  const netInfoText = await page.locator("#networkInfo").innerText();
  const backendAvailable = !String(netInfoText || "").toLowerCase().startsWith("load failed");

  // Trigger dirty-state (without saving) only when backend state is available.
  if (backendAvailable) {
    const slider = page.locator("#setLedBrightness");
    if (await slider.isVisible()) {
      const val = await slider.inputValue();
      const next = Math.max(1, Math.min(255, (Number(val) || 28) + 1));
      await slider.fill(String(next));
      await expect(page.locator("#settingsDirtyHint")).toContainText("Unsaved changes");
    }
  }

  // Safe action buttons in settings.
  await page.click("#btnNetReload");
  await expect(page.locator("#networkInfo")).toBeVisible();

  // Diagnostic relay test buttons (only click when enabled).
  if (backendAvailable) {
    const testFocus = page.locator("#btnTestFocus");
    const testTrigger = page.locator("#btnTestTrigger");
    if (!(await testFocus.isDisabled())) await testFocus.click();
    if (!(await testTrigger.isDisabled())) await testTrigger.click();
  }

  // Back to control and conditional START warning check.
  await page.click("#tabControl");
  await expect(page.locator("#btnStart")).toBeVisible();
  const ttState = parseBadgeState(await page.locator("#ttBadge").innerText());
  if (ttState === "off") {
    await page.click("#btnStart");
    await expect(page.locator("#startWarn")).toContainText("Turntable not connected");
  }

  await page.screenshot({ path: "ui-test-artifacts/ui-smoke.png", fullPage: true });
});
