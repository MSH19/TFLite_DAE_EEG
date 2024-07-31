
// include the model 
#include "examples/tflm_hello_world/hello_world_model.h"

// include libraries 
#include <cstdio>
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/all_ops_resolver.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"

// this code runs a tiny tensor flow model on the M7 core 
// build and flash from coralmicro root:
//  bash build.sh
//  python3 scripts/flashtool.py -e tflm_hello_world

// function to check for stack overflow 
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) 
{
  printf("Stack overflow in %s\r\n", pcTaskName);
}//end function 

// name space 
namespace 
{
  TickType_t startTick, endTick;
  unsigned long executionTime;
  
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  // int inference_count = 0;
  const int kModelArenaSize = 45000;   // model size
  const int kExtraArenaSize = 80000;   // extra size for tensors 
  const int kTensorArenaSize = kModelArenaSize + kExtraArenaSize;
  uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)));
  
  // initialized as constant values for testing only 
  // const float zero_point = -128;
  // const float scale = 0.0038;
  
  // actual noisy input signal sample 
float x[800] = {0.440975876,0.406197657,0.421517045,0.470764277,0.510701062,0.509067038,0.477486161,0.460279838,0.486202344,0.534048619,0.553261373,0.5193428,0.462161717,0.433624559,0.45076223,0.480747085,0.48362993,0.460947753,0.450270642,0.473540269,0.503957238,0.494850965,0.437401178,0.377340451,0.369339661,0.421279006,0.491372285,0.536229273,0.550813134,0.557727485,0.568702139,0.571210985,0.552993214,0.52626623,0.515470815,0.525553649,0.534238919,0.52133714,0.495718535,0.483684362,0.492136912,0.495738762,0.467401217,0.415183323,0.378188447,0.383753264,0.417443054,0.442183381,0.44313949,0.444342009,0.476988398,0.537870981,0.586407139,0.584161537,0.53300269,0.47325961,0.446079515,0.459630732,0.487282416,0.494593224,0.468440539,0.426622058,0.402826892,0.419328573,0.467518437,0.513717162,0.527953107,0.511291402,0.494030674,0.504545608,0.540311259,0.572580695,0.578391619,0.563401642,0.549462538,0.545876027,0.540087377,0.519102763,0.489750853,0.469875687,0.461137805,0.442919835,0.400182454,0.351913159,0.340128016,0.386170286,0.46399745,0.52350681,0.539429688,0.531672581,0.537567929,0.571065769,0.613540536,0.639490648,0.641086238,0.627396363,0.608805138,0.592052167,0.585766299,0.598122921,0.622671843,0.634578116,0.614023081,0.575197457,0.561946119,0.604606292,0.682352799,0.737522736,0.730943263,0.679248869,0.631165964,0.610380106,0.589272669,0.523795336,0.409546581,0.297070856,0.250772332,0.294494603,0.39643473,0.499884564,0.565180131,0.587480299,0.586950384,0.589216389,0.609986199,0.647231268,0.681937279,0.69112954,0.667262251,0.628244522,0.603719112,0.608714439,0.629851698,0.63970247,0.623888682,0.592106153,0.563519634,0.545960175,0.533626661,0.522995533,0.523672698,0.547302946,0.587278966,0.616475172,0.610956231,0.575714129,0.542039382,0.536595744,0.55412827,0.565044214,0.549365217,0.51931637,0.502835439,0.506520912,0.501834444,0.456103498,0.376439765,0.315778799,0.326365133,0.405592545,0.491613639,0.518379538,0.475299262,0.412032638,0.384770831,0.401922354,0.424568526,0.417728486,0.39266576,0.394944909,0.450087686,0.527341494,0.561079915,0.511661137,0.406596032,0.322054476,0.320148094,0.398824103,0.497295776,0.551410194,0.546862858,0.524053694,0.53159184,0.576353759,0.618291259,0.614162128,0.562184307,0.501521973,0.468677119,0.462392051,0.454919463,0.433350855,0.41802379,0.435822464,0.48342291,0.529050055,0.551178238,0.563163836,0.589030629,0.621786993,0.622504475,0.569966462,0.4997674,0.47717743,0.526205099,0.597966785,0.620058318,0.574382208,0.5135864,0.498776575,0.531975187,0.56095751,0.544584622,0.498027571,0.467923923,0.475723387,0.4998189,0.511714634,0.512763432,0.525403667,0.556502021,0.584662424,0.586434377,0.563533409,0.536506107,0.519259105,0.512116409,0.518697235,0.55261692,0.616028299,0.678680158,0.696682524,0.657971246,0.59929644,0.56682441,0.563857997,0.551039183,0.501702705,0.44534755,0.443031235,0.521464616,0.641882118,0.73884601,0.780190345,0.782814642,0.777063979,0.77286849,0.765211742,0.758379371,0.766837962,0.791333436,0.808763224,0.79569919,0.75725491,0.721603264,0.705480263,0.695102493,0.665840991,0.613465421,0.556051402,0.509233989,0.472424787,0.445527588,0.446476162,0.496245718,0.583226816,0.656940188,0.670203388,0.627203521,0.581172101,0.58077852,0.623675302,0.665140265,0.666068563,0.626002255,0.574434204,0.542706369,0.54930109,0.59925222,0.67967104,0.753404773,0.773447895,0.72140409,0.633242873,0.574835851,0.582007385,0.625961095,0.643877358,0.603879926,0.536241836,0.500168225,0.525531164,0.589873915,0.646660454,0.668006801,0.660031534,0.646383343,0.645137518,0.658644281,0.676221559,0.681932396,0.664890971,0.628597865,0.591679004,0.574017642,0.580594885,0.599560187,0.617690439,0.633495771,0.652864373,0.674064913,0.684286101,0.673815436,0.649680321,0.631032646,0.632245795,0.654229109,0.691702316,0.740575712,0.793892919,0.834537647,0.842608577,0.814246897,0.76948468,0.736256033,0.728404594,0.7421765,0.770269858,0.809042396,0.848132064,0.863818723,0.837654339,0.785165319,0.75354001,0.778163003,0.842183205,0.88957748,0.883070194,0.842299123,0.818122876,0.834490311,0.867207561,0.881510063,0.87646121,0.879231537,0.900108858,0.911197786,0.879924409,0.815121818,0.764040596,0.761390946,0.792072794,0.812461049,0.801384642,0.778434214,0.772704273,0.788003307,0.807547702,0.823356172,0.843068934,0.865311009,0.865315484,0.821687545,0.754807953,0.720355236,0.753168157,0.824505562,0.867480982,0.845731218,0.786755463,0.743637681,0.734714393,0.734274121,0.718976729,0.704095425,0.720609479,0.76510775,0.792956568,0.770036238,0.719592982,0.701685082,0.743817061,0.805737436,0.822729573,0.779962532,0.731611766,0.742291469,0.817073978,0.896755175,0.920523359,0.884676782,0.837437036,0.8233097,0.840480302,0.852059812,0.830568016,0.786426846,0.756158528,0.768988021,0.824645488,0.896004854,0.947341608,0.95315455,0.909223927,0.832959839,0.75334641,0.694641818,0.664514948,0.654788535,0.652318102,0.649770439,0.647936243,0.65242021,0.669815742,0.704850959,0.754967425,0.805437161,0.833769564,0.826094672,0.792612554,0.763704797,0.765475418,0.796623728,0.830773138,0.842769763,0.832463903,0.821330359,0.828388991,0.851685941,0.872928551,0.875378347,0.854488023,0.816063437,0.772811197,0.74516447,0.755679772,0.808954381,0.873885949,0.897359154,0.850273894,0.762388343,0.701670932,0.708057989,0.749689972,0.753774275,0.685825014,0.591363806,0.551135456,0.595323919,0.671460488,0.701129183,0.661979144,0.604423733,0.589323698,0.621277109,0.651864685,0.641400218,0.603397827,0.583335248,0.605026299,0.64933198,0.68386496,0.698933647,0.706791625,0.714254801,0.712660741,0.697225315,0.684831707,0.698858942,0.73835865,0.773132987,0.776277611,0.755041672,0.741719642,0.754880543,0.778370561,0.782577934,0.760193787,0.733481081,0.728253578,0.748488253,0.77927347,0.806116818,0.82317544,0.825954137,0.808588356,0.775480719,0.749426346,0.755694588,0.79461443,0.835700748,0.84530909,0.818093358,0.777706909,0.748168913,0.731912481,0.716607526,0.695030236,0.669260006,0.639441092,0.601318036,0.562109819,0.549473005,0.589028033,0.669506228,0.740322595,0.75678325,0.727701963,0.708895496,0.744688533,0.819296203,0.872830876,0.862012579,0.799431774,0.731880612,0.690114109,0.66714872,0.6420067,0.611770506,0.594278223,0.604830079,0.639676798,0.682755789,0.720576463,0.748065204,0.765579806,0.780041142,0.806918143,0.860266886,0.93305667,0.991573753,1,0.954920086,0.892010649,0.853138921,0.847953428,0.850776303,0.833439516,0.796052567,0.763437311,0.757721987,0.778909273,0.810115457,0.8334364,0.839580664,0.830969408,0.822470613,0.834851237,0.872536011,0.902461774,0.866265461,0.732291281,0.542448578,0.397242585,0.378614583,0.48126812,0.621141928,0.711881327,0.73041567,0.711066277,0.691920994,0.681987823,0.679260491,0.698035999,0.758294019,0.849065341,0.921692083,0.934688102,0.900262282,0.873385352,0.890579353,0.929591488,0.937859184,0.895181362,0.835602832,0.804742915,0.807312992,0.807481783,0.777782644,0.733319959,0.712697171,0.733518361,0.778037949,0.820014018,0.852426432,0.881157267,0.900692753,0.890750227,0.841531876,0.773710083,0.724249897,0.713512674,0.730692936,0.749561682,0.750414126,0.725351151,0.674552171,0.610750199,0.565962392,0.575801549,0.643564158,0.72297966,0.753250952,0.719546758,0.672404565,0.677140962,0.744096735,0.817706795,0.83635651,0.794351576,0.736276549,0.697577551,0.667881407,0.6209445,0.567309603,0.556176186,0.618249058,0.719263238,0.785828943,0.778922927,0.731736253,0.709081143,0.736593149,0.779649311,0.789984221,0.760585088,0.725850622,0.714607094,0.71684475,0.703246254,0.669107102,0.646312137,0.667601319,0.725120173,0.772679514,0.771438965,0.730982575,0.701426633,0.723948793,0.789280513,0.845484624,0.847819579,0.800568034,0.749736607,0.733791343,0.744371565,0.738859239,0.691674845,0.62708015,0.594378511,0.612596776,0.649156459,0.660127643,0.644888108,0.645778928,0.687972665,0.733035151,0.71150514,0.607038154,0.49275208,0.46604754,0.545946082,0.647615759,0.668485855,0.592178839,0.497053261,0.464562789,0.495297668,0.524023036,0.504463811,0.458164693,0.435020714,0.444612158,0.446972665,0.409125685,0.350751715,0.32099303,0.339432594,0.376562227,0.393937512,0.387105588,0.377993182,0.375375175,0.363449752,0.333751236,0.310603461,0.325893842,0.371368732,0.395145641,0.358145495,0.284552955,0.241504435,0.265796713,0.324127285,0.352001343,0.323506649,0.267862299,0.220479156,0.177428653,0.113742861,0.037142408,0,0.044557973,0.146594386,0.230941896,0.247455673,0.22030307,0.214112489,0.258295315,0.318859231,0.345663259,0.333623619,0.322502369,0.340893549,0.366367498,0.35401693,0.297167369,0.245947722,0.256674903,0.327854784,0.394424242,0.388501048,0.304627606,0.205796381,0.167816855,0.217439828,0.315537501,0.392319479,0.398610851,0.3354221,0.246105424,0.183282132,0.173777753,0.20518566,0.241397837,0.254168479,0.243017144,0.227333091,0.220554905,0.215822305,0.198422969,0.170164133,0.154048136,0.171299005,0.213655815,0.244856192,0.232401264,0.179571347,0.123603066,0.102477649,0.122572214,0.157900844,0.177887678,0.175417882,0.169666157,0.185658672,0.230222566,0.285284551,0.321637858,0.322562027,0.298421501,0.280145077,0.293227779,0.333547218,0.36845821,0.368088361,0.337708983,0.316696339,0.337567117,0.385704405,0.405848346,0.356829324,0.260398605,0.189423762,0.200854579,0.279608717,0.351967577,0.356163477,0.299400089,0.246322642,0.255464338,0.32930115,0.421372403,0.482044234,0.493400478,0.467970551,0.427664213,0.390503576,0.368839447,0.366218889,0.37036295,0.35691782,0.311246051,0.25000677,0.214845587,0.236716878,0.304662336,0.371407258,0.393573244,0.367070086};
  
  // loop
  static void loop() 
  {
    // in each loop we invoke inference on one input signal 
    printf("Loop\r\n");
    
    // input 
    // float input_value = 0.44;

    // fill the input tensor (800)
    for (int i = 0; i < 800; i++) 
    {
      // printf("%d\r\n",i);
      // int8_t x_quantized = static_cast<int8_t>(input_value/scale + zero_point);
      int8_t x_quantized = static_cast<int8_t>(x[i] / input->params.scale + input->params.zero_point);
      input->data.int8[i] = x_quantized;
      vTaskDelay(pdMS_TO_TICKS(1)); // Delay for 1 milliseconds
    }//end for 
    
    // Record the start time
    startTick = xTaskGetTickCount();
    
    
    
    // invoke inference 
    TfLiteStatus invoke_status = interpreter->Invoke();
    
     // Record the end time
    endTick = xTaskGetTickCount();

    // Calculate the execution time in ticks
    executionTime = endTick - startTick;
    
    if (invoke_status != kTfLiteOk) 
    { 
      TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed");
      return; 
    }//end if 

    // obtain the output from model's output tensor (800)
    // float y_val = (y_quantized - zero_point) * scale;
    // int8_t y_quantized = output->data.int8[0];
    // float y_val = (y_quantized - output->params.zero_point) * output->params.scale;
    // TF_LITE_REPORT_ERROR(error_reporter, "x_val: %f y_val: %f", static_cast<double>(x[0]), static_cast<double>(y_val));
    // printf("y_val: %.6f\n", static_cast<double>(y_val));
    
    
    // print the output tensor (800)
    for (int i = 0; i < 800; i++) 
    {
      int8_t y_quantized = output->data.int8[i];
      float y_val = (y_quantized - output->params.zero_point) * output->params.scale;
      printf("%.4f,", static_cast<double>(y_val));
      vTaskDelay(pdMS_TO_TICKS(1)); // Delay for 1 milliseconds
    }//end for
    printf("\n\r");
    
    // Print the execution time
    printf("Execution Time: %lu ticks\n", executionTime);

    // Optionally convert ticks to milliseconds
    executionTime = executionTime * portTICK_PERIOD_MS;
    printf("Execution Time: %lu ms\n", executionTime);
    
  }//end loop 

  [[noreturn]] void inference_task(void* param) 
  {
    printf("Starting inference task...\r\n");
    while (true) 
    {
      loop();
      taskYIELD();
    }//end while 
  }//end task 

} // end name space

extern "C" [[noreturn]] void app_main(void* param) 
{
  (void)param;
  printf("Tensor flow lite example!\r\n");
  
  // initialize reporter 
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  TF_LITE_REPORT_ERROR(error_reporter, "Reporter\n\r");
  
  // get model 
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) 
  {
    TF_LITE_REPORT_ERROR(error_reporter, "Model schema version is %d, supported is %d", model->version(), TFLITE_SCHEMA_VERSION);
    vTaskSuspend(nullptr);
  }//end if 
  
  // resolver 
  static tflite::AllOpsResolver resolver;
  
  // interpreter
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) 
  {
    TF_LITE_REPORT_ERROR(error_reporter, "Allocate tensors failed.");
    vTaskSuspend(nullptr);
  }//end 

  input = interpreter->input(0);
  output = interpreter->output(0);

  int ret;
  // the configMINIMAL_STACK_SIZE is set to 128 words = 512 bytes by default. 
  // we need to set our stack size sufficiently large to accomodate for the model operation (we can set it to 50 x 128 words =  25600 bytes)
  ret = xTaskCreate(inference_task, "InferenceTask", configMINIMAL_STACK_SIZE * 50, nullptr, configMAX_PRIORITIES - 1, nullptr);
  if (ret != pdPASS) 
  {
    printf("Create task failed \r\n");
  }//end if 
  
  while (true) 
  {
    taskYIELD();
  }//end while 
  
}//end name space 
