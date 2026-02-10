import { expect, test } from "@playwright/test";

test("@device settings API + UI roundtrip (LED brightness)", async ({ page, request }) => {
  const apiResp = await request.get("/api/network");
  if (!apiResp.ok()) {
    test.skip(true, "Backend API not available at /api/network");
  }

  const cfg = await apiResp.json();
  if (!cfg || !cfg.ok) {
    test.skip(true, "Device did not return valid network config");
  }

  await page.goto("/", { waitUntil: "domcontentloaded" });
  await page.click("#tabSettings");

  const slider = page.locator("#setLedBrightness");
  await expect(slider).toBeVisible();
  await expect(page.locator("#btnNetSave")).toBeVisible();

  const originalVal = Number(await slider.inputValue());
  const nextVal = originalVal >= 255 ? 254 : (originalVal + 1);

  await slider.fill(String(nextVal));
  await expect(page.locator("#settingsDirtyHint")).toContainText("Unsaved changes");

  await page.click("#btnNetSave");
  await expect(page.locator("#settingsDirtyHint")).toContainText("No unsaved changes");
  await expect(slider).toHaveValue(String(nextVal));

  await page.click("#btnNetReload");
  await expect(slider).toHaveValue(String(nextVal));

  // Restore original value to keep device settings unchanged after test.
  await slider.fill(String(originalVal));
  await page.click("#btnNetSave");
  await expect(page.locator("#settingsDirtyHint")).toContainText("No unsaved changes");
  await expect(slider).toHaveValue(String(originalVal));
  await page.click("#btnNetReload");
  await expect(slider).toHaveValue(String(originalVal));
});
