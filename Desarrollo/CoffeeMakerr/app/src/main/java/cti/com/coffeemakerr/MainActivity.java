package cti.com.coffeemakerr;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.DisconnectedBufferOptions;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import static java.lang.Double.parseDouble;

//mqtt

public class MainActivity extends AppCompatActivity {

    private Switch swEncender;
    private ImageView PicCafe;
    private ImageView PicServer;
    private TextView txtMensaje;

    MqttAndroidClient mqttAndroidClient;
    MqttConnectOptions mqttConnectOptions;

    private static final String USERNAME = "Prototipado";
    private final String PASSWORD = "S9i6r3pmQWME";
    final String BROKER = "tcp://m10.cloudmqtt.com:18485";

    final String readyMessage = "ready";

    private String _TOPESTADOCAFETERA = "/onoff";
    private String _TOPTEMPERATURA = "/temp/ir";
    private String _TOPPROX = "/prox";
    private String _TOPCAFETERA = "/cafetera";

    private boolean ready = false;
    private boolean appConectada = true;
    private boolean cafeEncendida = false;
    private int Tempera=0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //referencio a los controles
        PicCafe = (ImageView) findViewById(R.id.ImagenCafe);
        PicServer = (ImageView) findViewById(R.id.ImagenServer);
        swEncender = (Switch) findViewById(R.id.Encendida);
        txtMensaje = (TextView) findViewById(R.id.TextMensaje);

        swEncender.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (appConectada){
                    if (ready){
                        subscribeToTopic(_TOPTEMPERATURA);
                        if (isChecked)
                            publishMessage(_TOPCAFETERA,"on");
                        else
                            publishMessage(_TOPCAFETERA,"off");
                    }else{
                        publishMessage(_TOPCAFETERA,readyMessage);
                        txtMensaje.setText("Coffee maker is not ready, put the coffee pot if it is removed");
                        addToHistory("Is checking. Wait please");
                        swEncender.setChecked(false);
                    }
                }else{
                    addToHistory("You need to connected tha app to server");
                    swEncender.setChecked(false);
                }
            }
        });

        PicServer.setImageResource(R.drawable.reddisconn);

        final String clientId = MqttClient.generateClientId();
        mqttAndroidClient = new MqttAndroidClient(getApplicationContext(), BROKER, clientId);

        mqttConnectOptions = new MqttConnectOptions();
        mqttConnectOptions.setAutomaticReconnect(true);
        mqttConnectOptions.setCleanSession(false);
        mqttConnectOptions.setUserName(USERNAME);
        mqttConnectOptions.setPassword(PASSWORD.toCharArray());

        mqttAndroidClient.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean reconnect, String Broker) {

                if (reconnect) {
                    // Because Clean Session is true, we need to re-subscribe
                    subscribeTopics();
                    PicServer.setImageResource(R.drawable.redconn);
                }
            }

            @Override
            public void connectionLost(Throwable cause) {
                addToHistory(cause.toString());
                PicServer.setImageResource(R.drawable.reddisconn);
                cafeteraNoLista();
            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception {
                if (topic.equalsIgnoreCase(_TOPTEMPERATURA))
                    SetTemperatura(message);
                else if (topic.equalsIgnoreCase(_TOPESTADOCAFETERA))
                    CafeteraEstado(message);
                else if (topic.equalsIgnoreCase(_TOPPROX))
                    CafeteraProximidad(message);
                else if (topic.equalsIgnoreCase(_TOPCAFETERA))
                    CafeteraEncendida(message);
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
            }
        });
    }

    private void SetTemperatura(MqttMessage _Message) {
        Tempera = (int)(parseDouble(_Message.toString()) + 0.5d);
        txtMensaje.setText("Coffee's temperature: " + Tempera + " °C");
        if (Tempera <30)
            PicCafe.setImageResource(R.drawable.frio);
        else if(Tempera >= 30 && Tempera<45)
            PicCafe.setImageResource(R.drawable.enfriandose);
        else if(Tempera >= 45 && Tempera<65)
            PicCafe.setImageResource(R.drawable.mediocalentandose);
        else if(Tempera >= 65)
            PicCafe.setImageResource(R.drawable.caliente);
    }

    private void CafeteraEstado(MqttMessage message) {
        if (message.toString().equalsIgnoreCase("on")){
            cafeEncendida = true;
            swEncender.setChecked(true);
            cafeteraLista();
        }else if (message.toString().equalsIgnoreCase("off")) {
            cafeEncendida = false;
            swEncender.setChecked(false);
            cafeteraNoLista();
            txtMensaje.setText("Coffee Maker off");
            publishMessage(_TOPCAFETERA,readyMessage);
        }
    }

    private void cafeteraLista(){

        ready = true;
    }

    private void CafeteraProximidad(MqttMessage message) {
        if ( Integer.parseInt(message.toString()) == 0)
        {
            cafeteraNoLista();
            addToHistory("Coffee pot Remove");
            swEncender.setChecked(false);
        }
        else {
            cafeteraLista();
            if (cafeEncendida)
                swEncender.setChecked(true);
        }
    }

    private void CafeteraEncendida(MqttMessage message) {
        if (message.toString().equalsIgnoreCase("conectada")){
            txtMensaje.setText("The Coffee maker is connected, put the coffee pot if it is removed");
            publishMessage(_TOPPROX,readyMessage);
        }
    }

    private void addToHistory(String mainText){
        System.out.println("LOG: " + mainText);
        Toast.makeText(MainActivity.this, mainText, Toast.LENGTH_SHORT).show();
    }

    private void subscribeTopics() {
        subscribeToTopic(_TOPCAFETERA);
        subscribeToTopic(_TOPESTADOCAFETERA);
        subscribeToTopic(_TOPPROX);
        appConectada = true;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.Conectar:
                conectar();
                return true;
            case R.id.help:
                Ayuda();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    private void Ayuda() {
        addToHistory("The next version will update this module");
    }

    private void conectar() {
        try {
            //addToHistory("Connecting to " + serverUri);
            mqttAndroidClient.connect(mqttConnectOptions, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    DisconnectedBufferOptions disconnectedBufferOptions = new DisconnectedBufferOptions();
                    disconnectedBufferOptions.setBufferEnabled(true);
                    disconnectedBufferOptions.setBufferSize(100);
                    disconnectedBufferOptions.setPersistBuffer(false);
                    disconnectedBufferOptions.setDeleteOldestMessages(false);
                    mqttAndroidClient.setBufferOpts(disconnectedBufferOptions);

                    PicServer.setImageResource(R.drawable.redconn);

                    subscribeTopics();
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    cafeteraNoLista();
                    appConectada= false;
                }
            });

        } catch (MqttException ex){
            ex.printStackTrace();
        }

    }

    public void subscribeToTopic( final String subscriptionTopic){
        try {
            mqttAndroidClient.subscribe(subscriptionTopic, 0, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    addToHistory("Subscribe to: " + subscriptionTopic);
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    addToHistory("fail subscribe to: " + subscriptionTopic);
                }
            });
        } catch (MqttException ex){
            System.err.println("Exception whilst subscribing");
            ex.printStackTrace();
        }
    }

    public void unSubscribeToTopic (final String unSubscriptionTopic){
        try{
            mqttAndroidClient.unsubscribe(unSubscriptionTopic, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    addToHistory("Unsubscribe to: "+ unSubscriptionTopic);
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                }
            });
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }

    public void publishMessage(final String TOPIC, final String publishMessage){

        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(publishMessage.getBytes());
            mqttAndroidClient.publish(TOPIC, message);
            if(!mqttAndroidClient.isConnected()){
                //code for no connected
                addToHistory("fail to Connected");
            }
        } catch (MqttException e) {
            System.err.println("Error Publishing: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private void cafeteraNoLista() {
        unSubscribeToTopic(_TOPTEMPERATURA);
        PicCafe.setImageResource(R.drawable.desconectada);
        ready = false;
    }
}
