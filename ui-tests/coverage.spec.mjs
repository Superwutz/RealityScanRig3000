import { expect, test } from "@playwright/test";

test("covers major UI sections and core non-destructive interactions", async ({ page, baseURL }) => {
  await page.goto(baseURL || "/", { waitUntil: "domcontentloaded" });

  await expect(page.locator("#tabControl")).toBeVisible();
  await expect(page.locator("#tabSettings")).toBeVisible();
  await expect(page.locator("#btnStart")).toBeVisible();
  await expect(page.locator("#btnPause")).toBeVisible();
  await expect(page.locator("#btnAbort")).toBeVisible();
  await expect(page.locator("#btnTriggerToggle")).toBeVisible();
  await expect(page.locator("#btnAutoApplyToggle")).toBeVisible();

  // Preset coverage
  await expect(page.locator("#presetSmall")).toBeVisible();
  await expect(page.locator("#presetMedium")).toBeVisible();
  await expect(page.locator("#presetLarge")).toBeVisible();
  await page.click("#presetMedium");
  await expect(page.locator("#presetMedium")).toHaveClass(/selected/);

  // Parameter panel coverage
  const panel = page.locator("#paramPanel");
  await expect(panel).toBeVisible();
  await page.click("#paramPanel summary");
  await expect(page.locator("#inpRotSteps")).toBeVisible();
  await expect(page.locator("#btnSetRotSteps")).toBeVisible();

  // UI-only toggle behavior
  const triggerTextBefore = await page.locator("#btnTriggerToggle").innerText();
  await page.click("#btnTriggerToggle");
  const triggerTextAfter = await page.locator("#btnTriggerToggle").innerText();
  expect(triggerTextAfter).not.toEqual(triggerTextBefore);

  const autoTextBefore = await page.locator("#btnAutoApplyToggle").innerText();
  await page.click("#btnAutoApplyToggle");
  const autoTextAfter = await page.locator("#btnAutoApplyToggle").innerText();
  expect(autoTextAfter).not.toEqual(autoTextBefore);

  // Settings + firmware section coverage
  await page.click("#tabSettings");
  await expect(page.locator("#setWifiSsid")).toBeVisible();
  await expect(page.locator("#setLedEnable")).toBeVisible();
  await expect(page.locator("#btnTestFocus")).toBeVisible();
  await expect(page.locator("#btnTestTrigger")).toBeVisible();
  await expect(page.locator("#fwFile")).toBeVisible();
  await expect(page.locator("#btnFwUpload")).toBeVisible();
  await expect(page.locator("#manifestUrl")).toBeVisible();
  await expect(page.locator("#btnCheckUpdate")).toBeVisible();
  await expect(page.locator("#btnFwUpdateNow")).toBeVisible();
});
