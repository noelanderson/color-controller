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
`TouchDispatcher` owns pointer capture and `UiActionProcessor` owns application side effects.

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

    Wheel --> Router["UiActionProcessor<br/>application side effects"]
    Presets --> Router
    Modes --> Router
    Power --> Router
    Slider --> Router

    Router --> Model["ControllerModel"]
    Router --> Lighting["LightingOutput"]
    Router --> Persistence["ColorPersistenceService"]
    Router --> Audio["AudioFeedback"]
    Router --> Renderer["UiRenderer"]

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
    participant Actions as UiActionProcessor
    participant Model as ControllerModel
    participant Lighting as LightingOutput
    participant Persistence as ColorPersistenceService
    participant Renderer as UiRenderer

    User->>Touch: press or drag on wheel
    Touch->>Interaction: touch sample x, y
    Interaction->>Dispatcher: update(touching, point, now)
    Dispatcher->>Controls: enumerate registered controls
    Dispatcher->>Wheel: tryTouchDown(event)
    Wheel-->>Dispatcher: claimed + SelectColor action
    Dispatcher->>Wheel: touchMove(event) while captured
    Wheel-->>Actions: SelectColor action
    Actions->>Model: select(color)<br/>mode = Solid
    Actions->>Persistence: noteManualColor(color, now)
    Actions->>Lighting: apply(now)
    Actions->>Renderer: drawDynamicUi(previewColor)
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
    participant Actions as UiActionProcessor
    participant Model as ControllerModel
    participant Persistence as ColorPersistenceService
    participant Renderer as UiRenderer
    participant Audio as AudioFeedback

    User->>Interaction: press P1-P4
    Interaction->>Dispatcher: update(touching, point, now)
    Dispatcher->>Preset: tryTouchDown(event)
    Preset->>Gesture: begin(now)

    loop each touch poll while held
        Interaction->>Preset: tick(now)
        Preset->>Gesture: update(now, inside, 700 ms)
    end

    Gesture-->>Actions: StorePreset(index), once
    Actions->>Model: storePreset(index)<br/>mode = Solid
    Actions->>Persistence: queuePresetSave(index, color)
    Actions->>Renderer: notify preset ID (PresetSaved, expiry)
    Renderer->>Preset: notify(notification)
    Actions->>Renderer: drawDynamicUi(...)
    Actions->>Audio: beepLong()

    User->>Interaction: release
    Interaction->>Dispatcher: consecutive missing samples
    Dispatcher->>Preset: touchUp(lastPoint, now)
    Preset-->>Actions: None<br/>store suppresses recall
```

### Microphone status and Music

```mermaid
sequenceDiagram
    participant Setup
    participant Audio as AudioFeedback task
    participant UI as Effect UI task
    participant Renderer as UiRenderer
    participant Music as ModeButtonControl

    Setup->>Audio: start codec task
    Note over Music: Initializing draws no red dot
    Audio->>Audio: initialize codec, I2S, and microphone

    alt initialization succeeds
        Audio-->>UI: status = Ready
        UI->>Renderer: redraw Music
        Renderer->>Music: draw context without fault dot
    else initialization fails
        Audio-->>UI: status = Unavailable
        UI->>Renderer: redraw Music
        Renderer->>Music: draw context with red fault dot
    end
```

## Runtime task interaction

Arduino `loop()` only polls SimpleAwait. Each steady-state task suspends for a
positive duration so the scheduler can idle and ESP-IDF work remains responsive.

```mermaid
flowchart TB
    Loop["Arduino loop()<br/>simpleawait::poll()"] --> Scheduler["SimpleAwait scheduler"]

    Scheduler --> TouchTask["Touch task<br/>5 ms"]
    Scheduler --> EffectTask["LED effect task<br/>25 ms"]
    Scheduler --> UiTask["Effect UI task<br/>200 ms"]
    Scheduler --> PersistenceTask["Persistence task<br/>250 ms"]
    Scheduler --> AudioTask["Audio task<br/>5 ms when idle"]

    TouchTask --> Interaction["InteractionController"]
    Interaction --> Dispatcher["TouchDispatcher"]
    Dispatcher --> Controls["InteractiveControls"]
    Controls --> Actions["UiActionProcessor"]
    Actions --> Model["ControllerModel"]
    Actions --> Lighting["LightingOutput"]
    Actions --> Renderer["UiRenderer"]

    EffectTask --> Lighting
    UiTask --> Renderer
    PersistenceTask --> Persistence["ColorPersistenceService"]
    AudioTask --> Audio["ES8311 + I2S"]
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

## Ownership rules

1. Controls own geometry, hit testing, mapping, drawing, and local interaction state.
2. Controls emit values or semantic events; they do not write NVS or operate LEDs/audio.
3. Every interactive control receives touch-down data and decides whether to claim it.
4. `TouchDispatcher` preserves the sole claimant through release debounce; layout rules prohibit overlapping controls.
5. `UiActionProcessor` converts control actions into application side effects.
6. `ControllerModel` is the source of truth for selected color, brightness, power, mode, and presets.
7. `UiRenderer` supplies a consistent render context and coordinates complete framebuffer transfers;
   `UiScene` owns ordered element drawing.
8. The sketch owns and visibly composes individual controls, registering interactive elements with
   `InteractiveControls` and drawable elements with `UiScene`.
9. Both collections store interface pointers only and have no knowledge of concrete control types.
10. The sketch passes geometry into each element, which uses the same bounds for drawing and hit testing.
11. Drawable-only elements such as the preview participate in scene order without appearing in the
    touch collection.
