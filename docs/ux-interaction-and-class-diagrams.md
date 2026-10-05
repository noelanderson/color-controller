# UX interaction and class diagrams

These diagrams describe the touch UI in `src/ColorController/`. They show which
component owns each interaction, how semantic events reach controller services,
and where rendering and state live.

## Screen layout

```mermaid
flowchart TB
    Preview["Live color preview<br/>solid selection or effect color"]

    subgraph Main["480 x 320 touch screen"]
        direction LR
        Wheel["Color wheel<br/>drag to select hue and saturation"]

        subgraph Presets["Preset and mode controls"]
            direction TB
            Row1["P1 &nbsp;&nbsp;&nbsp; P2"]
            Row2["P3 &nbsp;&nbsp;&nbsp; P4"]
            Row3["Rainbow &nbsp;&nbsp;&nbsp; Music"]
        end
    end

    subgraph Footer["Output controls"]
        direction LR
        Power["Power<br/>ON / OFF"]
        Brightness["Brightness slider<br/>0-255"]
    end

    Preview --> Main
    Main --> Footer
```

| Element | Touch behavior | Owned by |
|---|---|---|
| Color wheel | Press and drag immediately select a solid color | `ColorWheelControl` |
| P1-P4 | Tap recalls; hold inside for 700 ms stores; release outside cancels | `PresetButtonControl` |
| Rainbow | Release inside selects rainbow mode | `ModeButtonControl` |
| Music | Release inside selects music mode | `ModeButtonControl` |
| Power | Release inside toggles output | `PowerButtonControl` |
| Brightness | Press and drag update brightness | `BrightnessSliderControl` |
| Preview strip | Displays the selected or current effect color | `ColorPreviewControl` |

## Touch contact lifecycle

The touch controller can briefly report no contact while a finger is still on
the panel. Release debounce keeps that noise from splitting one gesture into
multiple presses.

```mermaid
stateDiagram-v2
    [*] --> Idle

    Idle --> Pressed: touch sample<br/>identify target and begin gesture
    Pressed --> Pressed: touch sample<br/>continue wheel or slider drag
    Pressed --> ReleaseDebouncing: first missing sample<br/>missing count = 1

    ReleaseDebouncing --> Pressed: touch returns<br/>continue same gesture
    ReleaseDebouncing --> ReleaseDebouncing: another missing sample
    ReleaseDebouncing --> Idle: missing count reaches threshold<br/>dispatch release and clear gesture
```

## Touch target and event routing

Controls own hit testing, value mapping, local gesture state, and drawing.
`TouchDispatcher` owns pointer capture. `InteractionController` returns the
resulting semantic actions to `monitorTouchInput()`, which publishes them to a
bounded queue. After startup restoration, the application coroutine is the only
caller that mutates `ControllerModel`.

```mermaid
flowchart LR
    Touch["ST77922 touch sample"] --> Reader["InteractionController<br/>sole hardware reader"]
    Reader --> Contact["TouchDispatcher<br/>debounce and pointer capture"]
    Contact --> Controls["InteractiveControls<br/>registered input elements"]

    Controls --> Claim["Each control inspects touch-down<br/>first claimant captures contact"]
    Claim --> Wheel["ColorWheelControl<br/>UiAction SelectColor"]
    Claim --> Presets["PresetButtonControl<br/>UiAction Recall or Store"]
    Claim --> Modes["ModeButtonControl<br/>UiAction ActivateMode"]
    Claim --> Power["PowerButtonControl<br/>UiAction TogglePower"]
    Claim --> Slider["BrightnessSliderControl<br/>UiAction SetBrightness"]

    Wheel --> Reader
    Presets --> Reader
    Modes --> Reader
    Power --> Reader
    Slider --> Reader
    Reader --> Producer["monitorTouchInput coroutine<br/>publishes returned actions"]
    Producer --> Queue["SimpleAwait Queue&lt;UiAction, 8&gt;<br/>ordered, bounded FIFO"]

    Queue --> App["Application coroutine<br/>UiActionProcessor"]
    App --> Model["ControllerModel<br/>sole steady-state mutation owner"]
    App --> Messages["ControllerMessages<br/>coalesced fixed-memory mailboxes"]
    Messages --> Lighting["Lighting coroutine<br/>sole LED writer"]
    Messages --> Persistence["Persistence coroutine<br/>sole NVS coordinator"]
    Messages --> Audio["Audio coroutine<br/>sole codec/I2S owner"]
    Messages --> Renderer["UI coroutine<br/>sole runtime renderer"]

    Renderer --> Scene["UiScene<br/>registered drawable elements"]
    Scene --> Controls
    Renderer --> Framebuffer["TFT_eSprite framebuffer"]
    Framebuffer --> Display["ST77922 display"]
```

### Non-overlapping touch regions

Interactive controls must not overlap. This is a layout requirement, not a
runtime arbitration feature. Registration order must never be used to decide
which of two visible controls receives a contact:

```mermaid
flowchart LR
    Change["Add or move a control"] --> Compare["Compare its touch region<br/>with every existing control"]
    Compare --> Overlap{"Any overlap?"}
    Overlap -- Yes --> Revise["Revise the layout"]
    Revise --> Compare
    Overlap -- No --> Register["Register the control in setup()"]
    Register --> Verify["Build and run the target-board touch smoke test"]
```

## Preset and mode interaction state

Each `PresetButtonControl` combines its geometry and drawing with a
hardware-independent `PresetGesture`. The button emits intent only; it does not
change the model or perform I/O.

Moving outside pauses hold eligibility and prevents an outside release from
recalling. Returning inside before release resumes the same gesture; if the
total elapsed hold time has reached 700 ms, the preset stores immediately.

```mermaid
stateDiagram-v2
    [*] --> NoControl

    NoControl --> PresetPressed: press P1-P4
    NoControl --> ModePressed: press Rainbow or Music

    PresetPressed --> PresetPressed: move or wait less than 700 ms
    PresetPressed --> Stored: inside at 700 ms<br/>emit Store once
    PresetPressed --> NoControl: release inside<br/>emit Recall
    PresetPressed --> NoControl: release outside<br/>emit None

    Stored --> Stored: remain pressed
    Stored --> NoControl: release anywhere<br/>emit None

    ModePressed --> NoControl: release inside Rainbow<br/>emit Rainbow
    ModePressed --> NoControl: release inside Music<br/>emit Music
    ModePressed --> NoControl: release outside<br/>emit None
```

### Preset event side effects

```mermaid
flowchart TD
    Event{"UiAction"}

    Event -->|Recall P1-P4| Recall["Select stored color<br/>switch to Solid"]
    Event -->|Store P1-P4| Store["Copy selected color into preset<br/>switch to Solid"]
    Event -->|Rainbow| Rainbow["Set Rainbow mode<br/>cancel pending manual-color save"]
    Event -->|Music| Music["Set Music mode<br/>reset audio envelope<br/>cancel pending manual-color save"]
    Event -->|None| Noop["No application change"]

    Recall --> RecallEffects["Apply LEDs<br/>redraw UI<br/>schedule stable-color persistence<br/>single beep"]
    Store --> StoreEffects["Queue preset persistence<br/>show SAVED<br/>apply LEDs<br/>redraw UI<br/>double beep"]
    Rainbow --> ModeEffects["Apply LEDs<br/>redraw UI<br/>single beep"]
    Music --> ModeEffects
```

## Common interaction sequences

### Dragging the color wheel

```mermaid
sequenceDiagram
    actor User
    participant Touch as ST77922_TOUCH
    participant Interaction as InteractionController
    participant Dispatcher as TouchDispatcher
    participant Controls as InteractiveControls
    participant Wheel as ColorWheelControl
    participant TouchTask as monitorTouchInput
    participant Queue as Queue<UiAction, 8>
    participant App as Application task
    participant Actions as UiActionProcessor
    participant Model as ControllerModel
    participant Messages as ControllerMessages
    participant Lighting as Lighting task
    participant Persistence as Persistence task
    participant Renderer as UI task

    User->>Touch: press or drag on wheel
    Touch->>Interaction: touch sample x, y
    Interaction->>Dispatcher: update(touching, point, now)
    Dispatcher->>Controls: enumerate registered controls
    Dispatcher->>Wheel: tryTouchDown(event)
    Wheel-->>Dispatcher: claimed + SelectColor action
    Dispatcher->>Wheel: touchMove(event) while captured
    Wheel-->>Interaction: SelectColor action
    Interaction-->>TouchTask: returned action batch
    TouchTask->>Queue: send action<br/>suspend if queue is full
    Queue-->>App: receive action
    App->>Actions: process(action, now)
    Actions->>Model: select(color)<br/>mode = Solid
    Actions->>Messages: latest color + lighting/UI invalidation
    Messages-->>Persistence: latest manual color
    Messages-->>Lighting: apply latest model state
    Messages-->>Renderer: coalesced dynamic redraw
    Renderer->>Controls: draw preview, marker, presets,<br/>power, and brightness
    Renderer-->>User: flush complete framebuffer
```

### Holding a preset to store

```mermaid
sequenceDiagram
    actor User
    participant Interaction as InteractionController
    participant Dispatcher as TouchDispatcher
    participant Preset as PresetButtonControl
    participant Gesture as PresetGesture
    participant TouchTask as monitorTouchInput
    participant Queue as Queue<UiAction, 8>
    participant App as Application task
    participant Actions as UiActionProcessor
    participant Model as ControllerModel
    participant Messages as ControllerMessages
    participant Persistence as Persistence task
    participant Renderer as UI task
    participant Audio as Audio task

    User->>Interaction: press P1-P4
    Interaction->>Dispatcher: update(touching, point, now)
    Dispatcher->>Preset: tryTouchDown(event)
    Preset->>Gesture: begin(now)

    loop each touch poll while held
        Interaction->>Preset: tick(now)
        Preset->>Gesture: update(now, inside, 700 ms)
    end

    Gesture-->>Interaction: StorePreset(index), once
    Interaction-->>TouchTask: returned action batch
    TouchTask->>Queue: send action
    Queue-->>App: receive action
    App->>Actions: process(action, now)
    Actions->>Model: storePreset(index)<br/>mode = Solid
    Actions->>Messages: preset persistence request + double cue
    Messages-->>Persistence: latest value for preset slot
    Persistence->>Persistence: savePreset(index, color)
    Persistence-->>Messages: PresetSaved only after NVS success
    Messages-->>Renderer: confirmed preset saved notification
    Renderer->>Preset: notify(notification)
    Renderer->>Renderer: drawDynamicUi(...)
    Messages-->>Audio: double cue

    User->>Interaction: release
    Interaction->>Dispatcher: consecutive missing samples
    Dispatcher->>Preset: touchUp(lastPoint, now)
    Preset-->>Interaction: None<br/>store suppresses recall
```

### Microphone status and Music

```mermaid
sequenceDiagram
    participant Setup
    participant Audio as AudioFeedback task
    participant UI as Effect UI task
    participant Lighting as Lighting task
    participant Renderer as UiRenderer
    participant Music as ModeButtonControl

    Setup->>Audio: start codec task
    Note over Music: Initializing draws no red dot
    Audio->>Audio: initialize codec, I2S, and microphone

    alt initialization succeeds
        Audio-->>UI: status = Ready
        Audio-->>Lighting: latest microphone amplitude mailbox
        UI->>Renderer: redraw Music
        Renderer->>Music: draw context without fault dot
    else initialization fails
        Audio-->>UI: status = Unavailable
        UI->>Renderer: redraw Music
        Renderer->>Music: draw context with red fault dot
    end
```

## Runtime task interaction

Arduino `loop()` only calls `poll_and_wait()`. Six steady-state tasks plus the
transient audio tone child use at most seven of eight configured task slots,
leaving one genuine diagnostic/startup headroom slot. Every task suspends for a
positive duration or on the bounded action queue.

```mermaid
flowchart TB
    Loop["Arduino loop()<br/>simpleawait::poll_and_wait()"] --> Scheduler["SimpleAwait scheduler"]

    Scheduler --> TouchTask["Touch task<br/>5 ms"]
    Scheduler --> AppTask["Application task<br/>waits on UiAction queue"]
    Scheduler --> EffectTask["Lighting task<br/>25 ms"]
    Scheduler --> UiTask["UI task<br/>25 ms mailbox / 200 ms effect"]
    Scheduler --> PersistenceTask["Persistence task<br/>250 ms"]
    Scheduler --> AudioTask["Audio task<br/>5 ms when idle"]

    TouchTask --> Interaction["InteractionController"]
    Interaction --> Dispatcher["TouchDispatcher"]
    Dispatcher --> Controls["InteractiveControls"]
    Controls --> Queue["Queue&lt;UiAction, 8&gt;"]
    Queue --> AppTask
    AppTask --> Actions["UiActionProcessor"]
    Actions --> Model["ControllerModel"]
    Actions --> Messages["ControllerMessages"]

    Messages --> EffectTask
    Messages --> UiTask
    Messages --> PersistenceTask
    Messages --> AudioTask
    EffectTask --> Lighting["LightingOutput"]
    UiTask --> Renderer["UiRenderer"]
    PersistenceTask --> Persistence["ColorPersistenceService"]
    AudioTask --> Audio["ES8311 + I2S"]
    AudioTask --> Levels["Latest microphone level"]
    Levels --> Lighting
    Lighting --> LEDs["AddressableLedStrip"]
```

## Class diagrams

The editable PlantUML sources are stored beside the generated SVGs in
[`docs/diagrams/`](diagrams/).

### Touch UI controls

[![Touch UI control architecture](diagrams/ui-control-classes.svg)](diagrams/ui-control-classes.svg)

Source: [`ui-control-classes.puml`](diagrams/ui-control-classes.puml)

### Runtime and services

[![Controller runtime and service architecture](diagrams/runtime-classes.svg)](diagrams/runtime-classes.svg)

Source: [`runtime-classes.puml`](diagrams/runtime-classes.puml)

## UML communication diagrams

These diagrams number the messages exchanged between runtime objects. They
complement the sequence diagrams by emphasizing object links, coroutine
ownership, backpressure, and mailbox semantics.

### Touch action to output

[![Touch action communication](diagrams/touch-action-communication.svg)](diagrams/touch-action-communication.svg)

Source: [`touch-action-communication.puml`](diagrams/touch-action-communication.puml)

### Confirmed preset persistence

[![Confirmed preset-save communication](diagrams/preset-save-communication.svg)](diagrams/preset-save-communication.svg)

Source: [`preset-save-communication.puml`](diagrams/preset-save-communication.puml)

### Audio feedback and microphone sampling

[![Audio communication](diagrams/audio-communication.svg)](diagrams/audio-communication.svg)

Source: [`audio-communication.puml`](diagrams/audio-communication.puml)

## Ownership rules

1. Controls own geometry, hit testing, mapping, drawing, and local interaction state.
2. Controls emit values or semantic events; they do not write NVS or operate LEDs/audio.
3. Every interactive control receives touch-down data and decides whether to claim it.
4. `TouchDispatcher` preserves the sole claimant through release debounce; layout rules prohibit overlapping controls.
5. `InteractionController` returns actions without mutating application state;
   `monitorTouchInput()` publishes them to the bounded FIFO and owns backpressure.
6. The application coroutine is the sole caller of `UiActionProcessor` and the sole steady-state
   writer of `ControllerModel`; startup restoration writes the model before runtime tasks spawn.
7. `UiActionProcessor` publishes fixed-memory messages instead of performing device I/O.
8. `ControllerMessages` coalesces continuous state, preserves one persistence request per preset,
   and retains the strongest pending audio cue.
9. The lighting, UI, persistence, and audio coroutines exclusively own their runtime hardware or
   service adapters.
10. `ControllerModel` is the source of truth for selected color, brightness, power, mode, and presets.
11. `UiRenderer` supplies a consistent render context and coordinates complete framebuffer transfers;
   `UiScene` owns ordered element drawing.
12. The sketch owns and visibly composes individual controls, registering interactive elements with
   `InteractiveControls` and drawable elements with `UiScene`.
13. Both collections store interface pointers only and have no knowledge of concrete control types.
14. The sketch passes geometry into each element, which uses the same bounds for drawing and hit testing.
15. Drawable-only elements such as the preview participate in scene order without appearing in the
    touch collection.
