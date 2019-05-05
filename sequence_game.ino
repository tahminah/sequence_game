

const int PIN_BUTTON = 2;
const int PIN_LED = 13;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_LED, OUTPUT);
}

int prev_button_state = -1;
unsigned long button_down_ts = 0;
  
void loop() {
  int button_state = digitalRead(PIN_BUTTON);
  unsigned long now = millis();

  if (button_state != prev_button_state) {
//    Serial.println(button_state);
    if (button_state == 1) {
      button_down_ts = now;
      button_down(PIN_LED);
    } else {
      button_up(PIN_LED, now - button_down_ts);
    }
  }

//  delay(10);
  prev_button_state = button_state;
}

const int GAME_STATE_NONE = 0;
const int GAME_STATE_PLAYING = 1;
const int GAME_STATE_INPUT = 2;
const int GAME_STATE_INFO = 3;

int state = GAME_STATE_NONE;

void button_down(int id) {
  if (state == GAME_STATE_NONE) {
    state = GAME_STATE_PLAYING;
    start_game();
  } else if (state == GAME_STATE_INPUT) {
    digitalWrite(PIN_LED, HIGH);
    user_input_performed(id);
  }
}

void button_up(int id, int duration) {
  if (state == GAME_STATE_INPUT) {
    digitalWrite(PIN_LED, LOW); 
  }
}

struct sequence_item {
  int button_id;
  int duration;
  int wait_duration;

  sequence_item(int button_id = 0, int duration = 0, int wait_duration = 0) {
    this->button_id = button_id;
    this->duration = duration;
    this->wait_duration = wait_duration;
  }
};

struct sequence {
  public:
    int length;
    sequence_item *items;

    sequence(int length = 4): length(length) {
      this->items = new sequence_item[length];
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
      beep(100);
    }
  }
};

sequence seq(0);

void beep(int ms) {
  digitalWrite(PIN_LED, HIGH);
  delay(ms);
  digitalWrite(PIN_LED, LOW);
}


void blink(int times, int on = 50, int off = 100) {
  for (int i = 0; i < times; i++) {
    beep(on);
    if (i != times-1) {
      delay(off);
    }
  }
}

void start_game() {
  unsigned long now = millis();
  randomSeed(now);

  seq = sequence(5);

  for (int i = 0; i < seq.length; i++) {
    int r = 0;
    if (i != 0) {
      r = random(200, 2000);
    }
    Serial.println(r);
    seq.items[i] = sequence_item(PIN_BUTTON, 0, r); 
  }

  blink(3);

  delay(500);
  seq.play();
  delay(500);

  blink(3);
  
  state = GAME_STATE_INPUT;
}

struct user_input_sequence {
  public:
    unsigned long p_ts;
    int length;
    int current;
    sequence_item *inputs;

    user_input_sequence(int length = 0): length(length) {
      this->p_ts = millis();
      this->current = 0;
      for (int i = 0; i < length; i++) {
        this->inputs[i] = sequence_item();
      }
    }

    bool add_input(int button_id, int duration) {

      unsigned long now = millis();

      int wait_duration = 0;
      if (this->current != 0) {
        wait_duration = now - this->p_ts;
      }

      this->inputs[this->current] = sequence_item(button_id, duration, wait_duration);

//      this->inputs[this->current].button_id = button_id;
//      this->inputs[this->current].duration = duration;
//      this->inputs[this->current].wait_duration = wait_duration;
      
      this->p_ts = now;
      this->current += 1;
      if (this->current == this->length) {
        return true;
      }
      return false;
    }
};

user_input_sequence ui_seq = user_input_sequence(0);

void user_input_performed(int button_id) {
  bool finished = false;
  if (ui_seq.length == 0) {
    ui_seq = user_input_sequence(seq.length);
  }
  finished = ui_seq.add_input(button_id, 0);
  if (finished) {
    state = GAME_STATE_INFO;
    info_user();
  }
}

void info_user() {
  for (int i = 0; i < ui_seq.length; i++) {
    Serial.println(ui_seq.inputs[i].wait_duration);
  }
  blink(5);
  ui_seq = user_input_sequence(0);
  state = GAME_STATE_NONE;  
}
