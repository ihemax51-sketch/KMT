using System.IO;
using System.Security.Cryptography;
using System.Text;
using Newtonsoft.Json;

namespace KMTGuard.AdminDesktop.Services;

public sealed class AuthService
{
    private readonly string _path;

    public AuthService()
    {
        var root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "KMTGuardAdmin");
        Directory.CreateDirectory(root);
        _path = Path.Combine(root, "admins.json");
        EnsureDefaultAdmin();
    }

    public bool Verify(string username, string password)
    {
        var store = Load();
        var admin = store.Admins.FirstOrDefault(x => x.Username.Equals(username.Trim(), StringComparison.OrdinalIgnoreCase));
        if (admin is null)
            return false;

        return FixedTimeEquals(admin.PasswordHash, Hash(password, admin.Salt));
    }

    private void EnsureDefaultAdmin()
    {
        if (File.Exists(_path))
            return;

        var salt = Convert.ToBase64String(RandomNumberGenerator.GetBytes(16));
        var store = new AdminStore
        {
            Admins =
            [
                new AdminAccount
                {
                    Username = "owner",
                    DisplayName = "Owner",
                    Role = "Owner",
                    Salt = salt,
                    PasswordHash = Hash("admin123", salt)
                }
            ]
        };

        File.WriteAllText(_path, JsonConvert.SerializeObject(store, Formatting.Indented));
    }

    private AdminStore Load()
    {
        return JsonConvert.DeserializeObject<AdminStore>(File.ReadAllText(_path)) ?? new AdminStore();
    }

    private static string Hash(string password, string salt)
    {
        var bytes = SHA256.HashData(Encoding.UTF8.GetBytes($"{salt}:{password}"));
        return Convert.ToBase64String(bytes);
    }

    private static bool FixedTimeEquals(string a, string b)
    {
        var left = Convert.FromBase64String(a);
        var right = Convert.FromBase64String(b);
        return CryptographicOperations.FixedTimeEquals(left, right);
    }

    private sealed class AdminStore
    {
        public List<AdminAccount> Admins { get; set; } = [];
    }

    private sealed class AdminAccount
    {
        public string Username { get; set; } = string.Empty;
        public string DisplayName { get; set; } = string.Empty;
        public string Role { get; set; } = string.Empty;
        public string Salt { get; set; } = string.Empty;
        public string PasswordHash { get; set; } = string.Empty;
    }
}
