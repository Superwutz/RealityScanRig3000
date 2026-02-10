import { defineConfig } from "@playwright/test";

const baseURL = process.env.UI_URL || "http://127.0.0.1:8080";

export default defineConfig({
  testDir: "./ui-tests",
  timeout: 45_000,
  fullyParallel: false,
  retries: 0,
  reporter: [["list"]],
  use: {
    baseURL,
    headless: true,
    screenshot: "only-on-failure",
    trace: "retain-on-failure",
  },
  projects: [
    {
      name: "chromium",
      use: { browserName: "chromium" },
    },
  ],
});
