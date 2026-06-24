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
        return {}
    env_vars = {}
    with open(env_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            env_vars[key] = value.strip().strip('"').strip("'")
    return env_vars


def generate_header(env):
    project_dir = env.Dir("#").get_abspath()
    env_path = os.path.join(project_dir, ".env")
    header_path = os.path.join(project_dir, "src", "env_credentials.h")

    env_vars = load_env_file(env_path)

    ssid = env_vars.get("WIFI_SSID", "")
    password = env_vars.get("WIFI_PASSWORD", "")
    url = env_vars.get("SUPABASE_URL", "")
    key = env_vars.get("SUPABASE_KEY", "")
    
    # Touch screen flag
    touch_enabled = env_vars.get("TOUCH_SCREEN_ENABLED", "true").lower() != "false"
    
    # Database provider
    db_provider = env_vars.get("DATABASE_PROVIDER", "supabase").lower()
    firebase_url = env_vars.get("FIREBASE_URL", "")
    firebase_email = env_vars.get("FIREBASE_CLIENT_EMAIL", "")
    firebase_key = env_vars.get("FIREBASE_PRIVATE_KEY", "")

    # Build header content
    content = "#ifndef ENV_CREDENTIALS_H\n#define ENV_CREDENTIALS_H\n\n"
    
    def c_escape(s):
        return s.replace('\\', '\\\\').replace('"', '\\"')

    if ssid:
        content += '#define WIFI_SSID "{}"\n'.format(c_escape(ssid))
    if password:
        content += '#define WIFI_PASSWORD "{}"\n'.format(c_escape(password))
    if url:
        content += '#define SUPABASE_URL "{}"\n'.format(c_escape(url))
    if key:
        content += '#define SUPABASE_KEY "{}"\n'.format(c_escape(key))

    if firebase_url:
        content += '#define FIREBASE_URL "{}"\n'.format(c_escape(firebase_url))
    if firebase_email:
        content += '#define FIREBASE_CLIENT_EMAIL "{}"\n'.format(c_escape(firebase_email))
    if firebase_key:
        content += '#define FIREBASE_PRIVATE_KEY "{}"\n'.format(c_escape(firebase_key))

    content += '#define TOUCH_SCREEN_ENABLED {}\n'.format(1 if touch_enabled else 0)
    content += '#define DATABASE_PROVIDER_FIREBASE {}\n'.format(1 if db_provider == "firebase" else 0)

    for var, default in FIELD_DEFAULTS.items():
        val = env_vars.get(var, default)
        content += '#define {} "{}"\n'.format(var, c_escape(val))

    content += "\n#endif\n"

    # Write file if changed to prevent rebuilding everything unnecessarily
    if os.path.exists(header_path):
        with open(header_path, "r") as f:
            old_content = f.read()
        if old_content == content:
            return

    with open(header_path, "w") as f:
        f.write(content)
    print("Generated src/env_credentials.h")


Import("env")
generate_header(env)
