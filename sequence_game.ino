

const int PIN_BUTTON = 2;

const int PIN_LED_GREEN = 8;
const int PIN_LED_NEUTRAL = 9;
const int PIN_LED_RED = 10;

const int PIN_SOUND = 3;

#define INACCURACY 200

void setup() {
  Serial.begin(9600);
  pinMode(PIN_SOUND, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_NEUTRAL, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
}

int prev_button_state = 0;
unsigned long button_down_ts = 0;
  
void loop() {
  int button_state = digitalRead(PIN_BUTTON);
  unsigned long now = millis();

  if (button_state != prev_button_state) {
    
//    Serial.print(prev_button_state);
//    Serial.print(" -> ");
//    Serial.println(button_state);
    
    prev_button_state = button_state;
    
    if (button_state == 1) {
      button_down_ts = now;
      button_down(PIN_BUTTON);
    } else {
      button_up(PIN_BUTTON, now - button_down_ts);
    }
  }
  
  delay(5);
}

const int GAME_STATE_NONE = 0;
const int GAME_STATE_PLAYING = 1;
const int GAME_STATE_INPUT = 2;
const int GAME_STATE_INFO = 3;

int state = GAME_STATE_NONE;

struct statistics {
  int wins = 0;
  int loses = 0;
  void add(bool win) {
    if (win) { this->wins += 1; }
    else { this->loses += 1; }
  }
  void print() {
    Serial.print("wins: ");
    Serial.println(this->wins);
    Serial.print("loses: ");
    Serial.println(this->loses);
  }
  int sequence_length() {
    int a = 0;
    if (this->wins > this->loses) {
      a = random(this->wins - this->loses) / 2;
    }
    return 3 + a;
  }
} statistics;

void button_down(int id) {
  
  if (state == GAME_STATE_NONE) {
    state = GAME_STATE_PLAYING;
    start_game();
  } else if (state == GAME_STATE_INPUT) {
    digitalWrite(PIN_LED_NEUTRAL, HIGH);
    tone(PIN_SOUND, 500);
    user_input_performed(id);
  }
}

void button_up(int id, int duration) {
  digitalWrite(PIN_LED_NEUTRAL, LOW); 
  if (state == GAME_STATE_INPUT) {
    noTone(PIN_SOUND);
  }
}

int random_tone(int start = 3, int end = 8) {
  return 100 * random(3, 8);
}

struct sequence_item {
  int button_id;
  int duration;
  int wait_duration;
  int tone;

  sequence_item(int button_id = 0, int duration = 0, int wait_duration = 0, int tone = -1) {
    this->button_id = button_id;
    this->duration = duration;
    this->wait_duration = wait_duration;
    this->tone = tone == -1 ? random_tone() : tone;
  }
};

struct sequence {
  public:
    int length;
    sequence_item items[20];

    sequence(int length = 0): length(length) {
      for (int i = 0; i < length; i++) {
        this->items[i] = sequence_item();
      }
    };

    void play() {
      for (int i = 0; i < this->length; i++) {
        sequence_item *item = this->items+i;
        if (i != 0) {
          delay(item->wait_duration);
        }
        digitalWrite(PIN_LED_NEUTRAL, HIGH);
        tone(PIN_SOUND, item->tone);
        delay(50);
        digitalWrite(PIN_LED_NEUTRAL, LOW);
        noTone(PIN_SOUND);
      }
  }
};

sequence seq = sequence();

struct user_input_sequence {
  public:
    unsigned long p_ts;
    int length;
    int current;
    sequence_item inputs[20];

    user_input_sequence(int length = 0): length(length) {
      this->p_ts = millis();
      this->current = 0;
      for (int i = 0; i < length; i++) {
        this->inputs[i] = sequence_item();
      }
    }

    int add_input(int button_id, int duration) {

      unsigned long now = millis();

      int wait_duration = 0;
      if (this->current != 0) {
        wait_duration = now - this->p_ts;
      }

      this->inputs[this->current] = sequence_item(button_id, duration, wait_duration);
      
      this->p_ts = now;
      this->current += 1;
      return current-1;
    }

    bool is_finished() {
      return this->current == this->length;
    }
};

user_input_sequence ui_seq = user_input_sequence();


void start_game_info(int loop_times = 3) {
  int led_pin = PIN_LED_GREEN;
  int c = loop_times, k = 0;
  for (int i = 0; i < c*3; i++) {
    if (i != 0) { delay(30); };
    k = i % 3;
    if (k == 0) { led_pin = PIN_LED_GREEN; }
    else if (k == 1) { led_pin = PIN_LED_NEUTRAL; }
    else { led_pin = PIN_LED_RED; }
    tone(PIN_SOUND, 100*i);
    digitalWrite(led_pin, HIGH);
    delay(30);
    noTone(PIN_SOUND);
    digitalWrite(led_pin, LOW);
  }
}

void start_user_input_info() {
  digitalWrite(PIN_LED_GREEN, HIGH);
  tone(PIN_SOUND, 800);
  delay(300);
  digitalWrite(PIN_LED_GREEN, LOW);
  noTone(PIN_SOUND);
}

void start_game() {
  unsigned long now = millis();
  randomSeed(now);

  int sequence_length = statistics.sequence_length();

  seq = sequence(sequence_length);

  Serial.print("Game sequence length: ");
  Serial.println(sequence_length);
  for (int i = 0; i < sequence_length; i++) {
    int r = 0;
    if (i != 0) {
      r = random(200, 1000);
    }
//    Serial.println(r);
    seq.items[i] = sequence_item(PIN_BUTTON, 0, r, random_tone(3, 13)); 
  }
  ui_seq = user_input_sequence(sequence_length);

  start_game_info(sequence_length);
  
  delay(500);
  seq.play();
  delay(500);

  start_user_input_info();
    
  state = GAME_STATE_INPUT;
}

void user_input_performed(int button_id) {
  int seq_i = ui_seq.add_input(button_id, 0);
  tone(PIN_SOUND, seq.items[seq_i].tone);
  if (ui_seq.is_finished()) {
    state = GAME_STATE_INFO;
    delay(30);
    noTone(PIN_SOUND);
    delay(500);
    info_user_result();
  }
}


void info_user_fail() {
  digitalWrite(PIN_LED_RED, HIGH);
  tone(PIN_SOUND, 2000);
  delay(800);
  digitalWrite(PIN_LED_RED, LOW);
  noTone(PIN_SOUND);
  
}

void info_user_success() {
//  digitalWrite(PIN_LED_GREEN, HIGH);
  int tones[] = { 300, 500, 300, 800 };
  int durations[] = { 50, 100, 50, 500 };
  int pauses[] = { 20, 150, 50, 0 };

  for (int i = 0; i < 4; i++) {
    tone(PIN_SOUND, tones[i]);
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(durations[i]);
    noTone(PIN_SOUND);
    digitalWrite(PIN_LED_GREEN, LOW);
    delay(pauses[i]);
  }
//  digitalWrite(PIN_LED_GREEN, LOW);
//  noTone(PIN_SOUND);
}

void info_user_result() {
  bool is_ok = true;
  int diff = 0;
  Serial.println("Accuracy per button click: ");
  for (int i = 1; i < ui_seq.length; i++) {
    diff = ui_seq.inputs[i].wait_duration - seq.items[i].wait_duration;
    Serial.println(diff);
    if (abs(diff) > INACCURACY) {
      is_ok = false;
    }
  }
  
  statistics.add(is_ok);
  statistics.print();
  Serial.println();
  
  if (is_ok) {
    info_user_success();
  } else {
    info_user_fail();
  }

  
  state = GAME_STATE_NONE;  
}
