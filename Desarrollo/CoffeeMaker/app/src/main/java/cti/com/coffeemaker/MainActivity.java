package cti.com.coffeemaker;

import android.content.Context;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.android.service.MqttService;
import org.eclipse.paho.client.mqttv3.DisconnectedBufferOptions;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttMessageListener;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {
    private Switch swEncender;
    private ImageView PicCafe;
    private ImageView PicMensaje;
    private TextView txtMensaje;
    private TextView txtConeccion;

    MqttAndroidClient mqttAndroidClient;
    MqttConnectOptions mqttConnectOptions;

    private static final String USERNAME = "Prototipado";
    private final String PASSWORD = "S9i6r3pmQWME";
    final String BROKER = "tcp://m10.cloudmqtt.com:18485";

    final String clientId = "Aplicaion";
    final String subscriptionTopic = "/cafetera";
    final String publishTopic = "/cafetera";
    final String publishMessage = "ready";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        PicCafe = (ImageView) findViewById(R.id.ImagenCafe);
        PicMensaje = (ImageView) findViewById(R.id.ImagenCheck);
        swEncender = (Switch) findViewById(R.id.Encendida);
        txtMensaje = (TextView) findViewById(R.id.TextMensaje);
        txtConeccion = (TextView) findViewById(R.id.TextConeccion);
        //vibra = (Vibrator) getSystemService(VIBRATOR_SERVICE);

        txtConeccion.setText("La Aplicación no ha sido conectada con el servidor");

        swEncender.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            }
        });


        mqttAndroidClient = new MqttAndroidClient(getApplicationContext(), BROKER, clientId);


        mqttAndroidClient.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean reconnect, String Broker) {

                if (reconnect) {
                    // Because Clean Session is true, we need to re-subscribe
                    subscribeToTopic(subscriptionTopic);
                } else {

                }
            }

            @Override
            public void connectionLost(Throwable cause) {

            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception {
                addToHistory("Incoming message: " + new String(message.getPayload()));
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {

            }
        });


        //mqttConnectOptions.setAutomaticReconnect(true);
        mqttConnectOptions.setCleanSession(false);
        mqttConnectOptions.setUserName(USERNAME);
        mqttConnectOptions.setPassword(PASSWORD.toCharArray());
        try {
            //addToHistory("Connecting to " + serverUri);
            mqttAndroidClient.connect(mqttConnectOptions, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    subscribeToTopic(subscriptionTopic);

                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                }
            });


        } catch (MqttException ex){
            ex.printStackTrace();
        }
    }

    private void addToHistory(String mainText){
        System.out.println("LOG: " + mainText);
        Toast.makeText(MainActivity.this, mainText, Toast.LENGTH_SHORT).show();
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
        addToHistory("En la siguiente version se actualizará este módulo");
    }

    private void conectar() {

    }

    public void subscribeToTopic( final String subscriptionTopic){
        try {
            mqttAndroidClient.subscribe(subscriptionTopic, 0, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    addToHistory("Suscrito a: " + subscriptionTopic);
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    addToHistory("fallo Suscrito a: " + subscriptionTopic);
                }
            });
        } catch (MqttException ex){
            System.err.println("Exception whilst subscribing");
            ex.printStackTrace();
        }
    }

    public void publishMessage(final String publishMessage){

        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(publishMessage.getBytes());
            mqttAndroidClient.publish(publishTopic, message);
            if(!mqttAndroidClient.isConnected()){
                //code for no connected
                addToHistory("No se ha conectado");
            }
        } catch (MqttException e) {
            System.err.println("Error Publishing: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
