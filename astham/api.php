<?php

/*
 * This file should be encoded as UTF8 without signature
 *
 */

class Dictionary
{
    public $code;
    public $name;
    public $file;
    public $locale;
    function __construct($c, $n, $f, $l)
    {
        $this->code = $c;
        $this->name = $n;
        $this->file = $f;
        $this->locale = $l;
    }
};

$g_strOSName = php_uname();
$g_dictionaries = array();

if(strstr($g_strOSName, "Windows")==FALSE){
	define("WIN32", FALSE);
	define("DICTIONARY_PATH", "./assets/");
	define("EXECUTABLE_APP", "/usr/local/bin/anagrams");
    array_push($g_dictionaries, new Dictionary("en", "English", "english.fzi", "en_US.utf8"));
    array_push($g_dictionaries, new Dictionary("sv", "Swedish", "svenska.fzi", "sv_SE.utf8"));
    array_push($g_dictionaries, new Dictionary("da", "Danish", "dansk.fzi", "da_DK.utf8"));
}
else{
	define("WIN32", TRUE);
	define("EXECUTABLE_APP", "..\\Release\\anagrams.exe");
	define("DICTIONARY_PATH", "..\\anagrams\\assets\\");
    array_push($g_dictionaries, new Dictionary("en", "English", "english.fzi", "english"));
    array_push($g_dictionaries, new Dictionary("sv", "Swedish", "svenska.fzi", "swedish"));
    array_push($g_dictionaries, new Dictionary("da", "Danish", "dansk.fzi", "danish"));
}


require_once 'api.class.php';

date_default_timezone_set("Europe/Stockholm");

function utf16_to_utf8($str) {
	$c0 = ord($str[0]);
	$c1 = ord($str[1]);

	if ($c0 == 0xFE && $c1 == 0xFF) {
		$be = true;
	} else if ($c0 == 0xFF && $c1 == 0xFE) {
		$be = false;
	} else {
		return $str;
	}

	$str = substr($str, 2);
	$len = strlen($str);
	$dec = '';
	for ($i = 0; $i < $len; $i += 2) {
		$c = ($be) ? ord($str[$i]) << 8 | ord($str[$i + 1]) :
		ord($str[$i + 1]) << 8 | ord($str[$i]);
		if ($c >= 0x0001 && $c<= 0x007F) {
			$dec .= chr($c);
		} else if ($c > 0x07FF) {
			$dec .= chr(0xE0 | (($c >> 12) & 0x0F));
			$dec .= chr(0x80 | (($c >>  6) & 0x3F));
			$dec .= chr(0x80 | (($c >>  0) & 0x3F));
		} else {
			$dec .= chr(0xC0 | (($c >>  6) & 0x1F));
			$dec .= chr(0x80 | (($c >>  0) & 0x3F));
		}
	}
	return $dec;
}

function utf8_exec($cmd, &$output=null, &$return=null)
{
	//get current work directory
	$cd = getcwd();

	// on multilines commands the line should be ended with "\r\n"
	// otherwise if unicode text is there, parsing errors may occur
	//$cmd = "@echo off
	//@chcp 65001 > nul
	//@cd \"$cd\"
	//".$cmd;
	$cmd = "@echo off
			@chcp 65001 > nul
			$cmd";


	//create a temporary cmd-batch-file
	//need to be extended with unique generic tempnames
	$tempfile = 'php_exec.bat';
	// $rsltfile = 'php_rslt.txt';
	file_put_contents($tempfile,$cmd);

	//execute the batch
	// exec("start /b $tempfile > $rsltfile", $output, $return);
	// exec("$tempfile > $rsltfile", $output, $return);
	// $rslt = file_get_contents($rsltfile);
	// $output = explode("\n", $rslt);
	exec("$tempfile", $output, $return);
	
	// get rid of the last two lines of the output: an empty and a prompt
	// array_pop($output);
	// array_pop($output);

	//if only one line output, return only the extracted value
	if(count($output) == 1){
		$output = $output[0];
	}

	//delete the batch-tempfile
	// unlink($tempfile);
	// unlink($rsltfile);

	return $output;

}

class AsthamAPI extends API
{
	protected $User;

	public function __construct($request, $origin) {
		parent::__construct($request);

		$User = new stdclass();
		$User->name = "mattias";

		if(false){
			// Abstracted out for example
			$APIKey = new Models\APIKey();
			$User = new Models\User();

			if (!array_key_exists('apiKey', $this->request)) {
				throw new Exception('No API Key provided');
			}
			else if (!$APIKey->verifyKey($this->request['apiKey'], $origin)) {
				throw new Exception('Invalid API Key');
			}
			else if (array_key_exists('token', $this->request) &&
				!$User->get('token', $this->request['token'])) {
				throw new Exception('Invalid User Token');
			}
		}

		$this->User = $User;
	}

	/**
	* langs endpoint
    * returns the supported languages
	*/
	protected function langs() {
        global $g_dictionaries;
		if ($this->method == 'GET') {
            $result = array();
            $result['supported'] = array();
            foreach($g_dictionaries as $d){
                array_push($result['supported'], $d);
            }
            $result['success'] = true;
			return $result;
		}
		else {
			return "Only accepts GET requests";
		}
	}

	protected function mb_str_shuffle($str) {
		$tmp = preg_split("//u", $str, -1, PREG_SPLIT_NO_EMPTY);
		shuffle($tmp);
		return join("", $tmp);
	}
	
	protected function a(array $params){
        global $g_dictionaries;
		if ($this->method == 'GET') {
			$result = array();
			$expression = urldecode($this->verb);
			$outputArray = array();
			$locale = "Swedish";
			$dictionary = "svenska.fzi";
			if(isset($params) && isset($params[0])){
                foreach($g_dictionaries as $d){
                    if($d->code == $params[0]){
                        $dictionary = $d->file;
                        $locale = $d->locale;
                        break;
                    }
                }
			}
			$dictionary = DICTIONARY_PATH . $dictionary;

            if(!file_exists($dictionary)){
                throw new Exception("Cannot find dictionary $dictionary");
            }

			if(!file_exists(EXECUTABLE_APP)){
				for($i = 0; $i < 10; $i++){
					array_push($outputArray, $this->mb_str_shuffle($expression));
				}
			}
			else{
				$cmd = EXECUTABLE_APP . " $dictionary \"$expression\" -l $locale";
			    if(WIN32){
				    utf8_exec($cmd, $outputArray, $execresult);
			    }
			    else{
				    exec($cmd, $outputArray, $execresult);
			    }
                if($execresult != 0){
                    throw new Exception("Undefined error when running command:\n'$cmd'\nExecresult is '$execresult'");
                }
			}
			$result['anagrams'] = $outputArray;
			$result['success'] = true;
			return $result;
		} else {
			return "Only accepts GET requests";
		}
	}
	
	
	protected function x(array $params){
		$result = array();
		if($this->method == 'GET'){
	        $result['id'] = '666';
	        $result['name'] = 'warlock';
	        return $result;
		}
		else{
			
		}
	}
} // class AsthamAPI


if(php_sapi_name() == 'cli'){
	// fake a HTTP request
	$_SERVER['REQUEST_METHOD'] = 'GET';
    echo  "Testing languages supported:\n";
    $API = new AsthamAPI("langs", "localhost");
    echo $API->processAPI();
    exit;
    echo "Testing anagram generation:\n";
	$expression = "EXAMPLE";
	$language = "en";
	if(isset($argv[1])){
		$expression = $argv[1];
	}
	if(isset($argv[2])){
		$language = $argv[2];
	}
	$API = new AsthamAPI("a/$expression/$language", "localhost");
	echo $API->processAPI();
}
else{
	// Requests from the same server don't have a HTTP_ORIGIN header
	if (!array_key_exists('HTTP_ORIGIN', $_SERVER)) {
		$_SERVER['HTTP_ORIGIN'] = $_SERVER['SERVER_NAME'];
	}

	try {
		$API = new AsthamAPI($_REQUEST['request'], $_SERVER['HTTP_ORIGIN']);
		echo $API->processAPI();
	} catch (Exception $e) {
		echo json_encode(Array('error' => $e->getMessage()));
	}
}
