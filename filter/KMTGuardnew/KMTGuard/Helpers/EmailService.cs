using Serilog;
using System;
using System.Net;
using System.Net.Mail;
using System.Threading.Tasks;

namespace KMTGuard.Helpers
{
    public class EmailService
    {
        private const string DefaultSmtpServer = "smtp.gmail.com";
        private const int DefaultSmtpPort = 587;

        public async Task SendEmailAsync(string recipient, string subject, string body)
        {
            try
            {
                string smtpServer = Environment.GetEnvironmentVariable("KMTGUARD_SMTP_SERVER")
                    ?? Environment.GetEnvironmentVariable("KMTGUARD_SMTP_SERVER")
                    ?? DefaultSmtpServer;
                int smtpPort = int.TryParse(Environment.GetEnvironmentVariable("KMTGUARD_SMTP_PORT")
                        ?? Environment.GetEnvironmentVariable("KMTGUARD_SMTP_PORT"), out int configuredPort)
                    ? configuredPort
                    : DefaultSmtpPort;
                string? senderEmail = Environment.GetEnvironmentVariable("KMTGUARD_SMTP_EMAIL")
                    ?? Environment.GetEnvironmentVariable("KMTGUARD_SMTP_EMAIL");
                string? senderPassword = Environment.GetEnvironmentVariable("KMTGUARD_SMTP_PASSWORD")
                    ?? Environment.GetEnvironmentVariable("KMTGUARD_SMTP_PASSWORD");

                if (string.IsNullOrWhiteSpace(senderEmail) || string.IsNullOrWhiteSpace(senderPassword))
                {
                    Log.Warning("Email was not sent because SMTP credentials are not configured.");
                    return;
                }

                using (SmtpClient smtpClient = new SmtpClient(smtpServer))
                {
                    smtpClient.Port = smtpPort;
                    smtpClient.Credentials = new NetworkCredential(senderEmail, senderPassword);
                    smtpClient.EnableSsl = true;

                    using (MailMessage mailMessage = new MailMessage())
                    {
                        mailMessage.From = new MailAddress(senderEmail);
                        mailMessage.To.Add(recipient);
                        mailMessage.Subject = subject;
                        mailMessage.Body = body;
                        mailMessage.IsBodyHtml = true; // If your body is in HTML format

                        // Asenkron olarak e-posta gönderme işlemi
                        await smtpClient.SendMailAsync(mailMessage);

                        //Log.Warning("Email sent successfully.");
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error sending email: {ex.Message}");
            }
        }
    }
}
