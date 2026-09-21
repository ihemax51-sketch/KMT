using System.Windows;
using System.ComponentModel;
using System.Windows.Media.Animation;
using KMTGuard.AdminDesktop.Services;

namespace KMTGuard.AdminDesktop;

public partial class LoginWindow : Window
{
    private readonly AuthService _authService = new();
    private bool _closeCommitted;
    private bool _windowPresented;

    public LoginWindow()
    {
        InitializeComponent();
        PasswordBox.Focus();
    }

    private void LoginWindow_ContentRendered(object? sender, EventArgs e)
    {
        if (_windowPresented)
            return;

        _windowPresented = true;
        var easing = new CubicEase { EasingMode = EasingMode.EaseOut };
        BeginAnimation(OpacityProperty, new DoubleAnimation(0, 1, TimeSpan.FromMilliseconds(180)) { EasingFunction = easing });
        WindowScaleTransform.BeginAnimation(System.Windows.Media.ScaleTransform.ScaleXProperty,
            new DoubleAnimation(0.985, 1, TimeSpan.FromMilliseconds(180)) { EasingFunction = easing });
        WindowScaleTransform.BeginAnimation(System.Windows.Media.ScaleTransform.ScaleYProperty,
            new DoubleAnimation(0.985, 1, TimeSpan.FromMilliseconds(180)) { EasingFunction = easing });
    }

    private void LoginWindow_Closing(object? sender, CancelEventArgs e)
    {
        if (_closeCommitted)
            return;

        e.Cancel = true;
        IsEnabled = false;
        var fade = new DoubleAnimation(Opacity, 0, TimeSpan.FromMilliseconds(120));
        fade.Completed += (_, _) =>
        {
            _closeCommitted = true;
            Dispatcher.BeginInvoke(new Action(Close));
        };
        BeginAnimation(OpacityProperty, fade);
    }

    private void CloseWindowButton_Click(object sender, RoutedEventArgs e) => Close();

    private void LoginButton_Click(object sender, RoutedEventArgs e)
    {
        if (_authService.Verify(UsernameBox.Text, PasswordBox.Password))
        {
            AppSession.CurrentAdmin = UsernameBox.Text.Trim();
            DialogResult = true;
            Close();
            return;
        }

        ErrorText.Text = "Invalid username or password.";
        ErrorPanel.Visibility = Visibility.Visible;
    }
}
