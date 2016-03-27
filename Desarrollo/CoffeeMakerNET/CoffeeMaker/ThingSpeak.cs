using System;
using System.IO;
using System.Net;
using System.Text;
using System.Web;

namespace ThingSpeak
{
    public class ThingSpeak
    {
        private const string _url = "http://api.thingspeak.com/";
        private const string _APIKey = "QTY07M1JZZB9KC77";

        public static bool SendDataToThingSpeak(string[] Fields)
        {
            StringBuilder sbQS = new StringBuilder();

            sbQS.Append(_url + "update?key=" + _APIKey);
            if (Fields.Length > 8)
            {
                throw (new Exception("Can't Handle More than 8 Parameters"));
            }
            for (int i = 0; i < Fields.Length; i++)
            {
                sbQS.Append("&field" + (i + 1) + "=" + HttpUtility.UrlEncode(Fields[i]));
            }

            // The response will be a "0" if there is an error or the entry_id if > 0
            var TSResponse = Convert.ToInt16(PostToThingSpeak(sbQS.ToString()));

            return TSResponse > 0 ? true : false;
        }

        public static bool UpdateThingkSpeakStatus(string status)
        {
            StringBuilder sbQS = new StringBuilder();
            sbQS.Append(_url + "update?key=" + _APIKey + "&status=" + HttpUtility.UrlEncode(status));

            var TSResponse = Convert.ToInt16(PostToThingSpeak(sbQS.ToString()));

            return TSResponse > 0 ? true : false;
        }

        public static bool UpdateThingSpeakLocation(string TSLat, string TSLong, string TSElevation)
        {
            StringBuilder sbQS = new StringBuilder();
            sbQS.Append(_url + "update?key=" + _APIKey);

            if (TSLat != null) sbQS.Append("&lat=" + TSLat);
            if (TSLong != null) sbQS.Append("&long=" + TSLong);
            if (TSElevation != null) sbQS.Append("&elevation=" + TSElevation);

            var TSResponse = Convert.ToInt16(PostToThingSpeak(sbQS.ToString()));

            return TSResponse > 0 ? true : false;
        }

        private static string PostToThingSpeak(string QueryString)
        {
            StringBuilder sbResponse = new StringBuilder();
            byte[] buf = new byte[8192];

            // Hit the URL with the querystring and put the response in webResponse
            HttpWebRequest myRequest = (HttpWebRequest)WebRequest.Create(QueryString);
            HttpWebResponse webResponse = (HttpWebResponse)myRequest.GetResponse();
            myRequest.Proxy = proxyObject;
            try
            {
                Stream myResponse = webResponse.GetResponseStream();

                int count = 0;

                // Read the response buffer and return
                do
                {
                    count = myResponse.Read(buf, 0, buf.Length);
                    if (count != 0)
                    {
                        sbResponse.Append(Encoding.ASCII.GetString(buf, 0, count));
                    }
                }
                while (count > 0);
                return sbResponse.ToString();
            }
            catch (WebException ex)
            {
                System.Windows.Forms.MessageBox.Show(ex.Message);
                return "0";
            }

        }

    }
}
