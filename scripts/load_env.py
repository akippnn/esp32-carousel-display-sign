import os

def load_env_file(env_path):
    """Parse a .env file and load key-value pairs into os.environ."""
    if not os.path.exists(env_path):
        return
    with open(env_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            os.environ[key] = value.strip().strip('"').strip("'")

def load_env(env):
    """Load .env from project root and inject values as build flags."""
    project_dir = env.Dir("#").get_abspath()
    env_path = os.path.join(project_dir, ".env")
    load_env_file(env_path)

    ssid = os.environ.get("WIFI_SSID", "")
    password = os.environ.get("WIFI_PASSWORD", "")
    url = os.environ.get("SUPABASE_URL", "")
    key = os.environ.get("SUPABASE_KEY", "")

    if ssid:
        env.Append(CPPDEFINES=[("WIFI_SSID", '\\"{}\\"'.format(ssid))])
    env.Append(CPPDEFINES=[("WIFI_PASSWORD", '\\"{}\\"'.format(password))])
    if url:
        env.Append(CPPDEFINES=[("SUPABASE_URL", '\\"{}\\"'.format(url))])
    if key:
        env.Append(CPPDEFINES=[("SUPABASE_KEY", '\\"{}\\"'.format(key))])

Import("env")
load_env(env)
