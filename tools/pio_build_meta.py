import subprocess

Import("env")


def git_short_sha():
    try:
        sha = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
        return sha or "dev"
    except Exception:
        return "dev"


env.Append(
    CPPDEFINES=[
        ("SCANRIG_BUILD_GIT", '\\"%s\\"' % git_short_sha()),
    ]
)
