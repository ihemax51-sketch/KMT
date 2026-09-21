using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KMTGuard.AsyncServerManager
{
    public class TokenProvider
    {
        public int ID { get; set; }
        public DateTime Date { get; set; }
        public string ClientIP { get; set; }
        public string AgentIP { get; set; }
        public ushort AgentPort { get; set; }



        static readonly List<TokenProvider> Tokens = new List<TokenProvider>();
        static readonly object Locker = new object();
        static int LastTokenID = 0;

        public TokenProvider(string clientIP, string agentIP, ushort agentPort)
        {
            ClientIP = clientIP;
            AgentIP = agentIP;
            AgentPort = agentPort;

            Date = DateTime.Now;

            lock (Locker)//threadsafe
            {
                LastTokenID++;
                ID = LastTokenID;
                Tokens.Add(this);
            }
        }

        public static TokenProvider? GetToken(string clientIP)
        {
            TokenProvider? token;
            lock (Locker) //threadsafe
            {
                Tokens.RemoveAll(t => t.Date < DateTime.Now.AddSeconds(-10)); //Tokenlerin 10 saniye içinde kullanılması gerekli.
                token = Tokens.Find(t => t.ClientIP == clientIP);
                if (token != null)
                {
                    Tokens.Remove(token); //Her token tek kullanımlıktır.
                }
            }
            return token;
        }

    }
}
