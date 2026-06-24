import os

FIELD_DEFAULTS = {
    "SUPABASE_FIELD_FIRST_NAME": "first_name",
    "SUPABASE_FIELD_LAST_NAME": "last_name",
    "SUPABASE_FIELD_POSITION": "position",
    "SUPABASE_FIELD_START": "schedule_start",
    "SUPABASE_FIELD_END": "schedule_end",
}


def load_env_file(env_path):
    if not os.path.exists(env_path):
        return
    with open(env_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            os.environ[key] = value.strip().strip('"').strip("'")


def inject(env, name, value):
    env.Append(CPPDEFINES=[(name, '\\"{}\\"'.format(value))])


def load_env(env):
    project_dir = env.Dir("#").get_abspath()
    env_path = os.path.join(project_dir, ".env")
    load_env_file(env_path)

    ssid = os.environ.get("WIFI_SSID", "")
    password = os.environ.get("WIFI_PASSWORD", "")
    url = os.environ.get("SUPABASE_URL", "")
    key = os.environ.get("SUPABASE_KEY", "")

    if ssid:
        inject(env, "WIFI_SSID", ssid)
    inject(env, "WIFI_PASSWORD", password)
    if url:
        inject(env, "SUPABASE_URL", url)
    if key:
        inject(env, "SUPABASE_KEY", key)

    for var, default in FIELD_DEFAULTS.items():
        inject(env, var, os.environ.get(var, default))


Import("env")
load_env(env)
